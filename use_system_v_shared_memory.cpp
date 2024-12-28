#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <memory>
#include <print>
#include <span>
#include <string_view>

#include "lock_free/ring_buffer.hpp"
#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/process.hpp"
#include "unix/signal.hpp"
#include "unix/system_v/ipc/group_notifier.hpp"
#include "unix/system_v/ipc/semaphore_set.hpp"
#include "unix/system_v/ipc/shared_memory.hpp"

template <class Resource>
static void remove_resource(Resource *resource) noexcept
{
  const auto removed = resource->remove();
  std::println("resource removed: {}", removed.has_value());
  assert(removed);
}

template <class Resource>
static void destroy_resource(Resource *resource) noexcept
{
  resource->~Resource();
  std::println("resource destroyed");
}

template <class Resource>
using resource_remover_t =
    std::unique_ptr<Resource, unix::deleter<remove_resource<Resource>>>;

template <class Resource>
using resource_destroyer_t =
    std::unique_ptr<Resource, unix::deleter<destroy_resource<Resource>>>;

constexpr std::size_t message_capacity = 256;
constexpr std::size_t message_queue_capacity = 32;
using message_t = std::array<char, message_capacity>;
using message_queue_t =
    lock_free::ring_buffer<message_t, message_queue_capacity>;

struct process_info
{
  std::size_t created_process_count{0};
  bool is_child{false};
  bool is_producer{false};
  std::size_t group_id{0};
};

bool signal_readiness_to_parent(const unix::system_v::ipc::group_notifier
                                    &children_readiness_notifier) noexcept
{
  const auto readiness_signaled = children_readiness_notifier.notify_one();

  if (!readiness_signaled)
  {
    std::println("failed to send signal about readiness due to: {}",
                 unix::to_string(readiness_signaled.error()).data());
    return false;
  }
  return true;
}

bool consume_messages(
    const process_info &info,
    const unix::system_v::ipc::group_notifier &message_written_notifier,
    const unix::system_v::ipc::group_notifier &message_read_notifier,
    message_queue_t &message_queue, const unix::process_id_t &process_id,
    const std::atomic<bool> &done_flag,
    std::atomic<std::int32_t> &consumed_message_count) noexcept
{
  while (true)
  {
    std::println("wait for message");
    const auto message_written = message_written_notifier.wait_for_one();

    if (done_flag.load())
    {
      std::println("done flag received");
      break;
    }
    if (!message_written)
    {
      std::println(
          "failed to receive signal about message being written due to: {}",
          unix::to_string(message_written.error()).data());
      return false;
    }
    const auto message = message_queue.pop();
    consumed_message_count++;
    std::println("received message: {} by child with id: {}", message.data(),
                 process_id);
    const auto message_read = message_read_notifier.notify_one();

    if (!message_read)
    {
      std::println("failed to send signal about message being read due to: {}",
                   unix::to_string(message_read.error()).data());
      return false;
    }
  }
  return true;
}

bool wait_till_all_children_ready(const std::size_t created_process_count,
                                  const unix::system_v::ipc::group_notifier
                                      &children_readiness_notifier) noexcept
{
  using namespace unix::system_v;

  const auto readiness_signaled = children_readiness_notifier.wait_for_all();

  if (!readiness_signaled)
  {
    std::println("signal about child being ready not received due to: {}",
                 unix::to_string(readiness_signaled.error()).data());
    return false;
  }
  return true;
}

bool wait_till_production_start(
    const unix::system_v::ipc::group_notifier &producers_notifier) noexcept
{
  const auto start_production = producers_notifier.wait_for_one();

  if (!start_production)
  {
    std::println("failed to receive starting signal from the parent");
    return false;
  }
  return true;
}

bool notify_about_production_completion(
    const unix::system_v::ipc::group_notifier &producers_notifier) noexcept
{
  const auto production_done = producers_notifier.notify_one();

  if (!production_done)
  {
    std::println("failed to send signal about production being done");
    return false;
  }
  return true;
}

bool produce_messages(
    const process_info &info, const std::size_t message_count,
    const unix::system_v::ipc::group_notifier &message_read_notifier,
    const unix::system_v::ipc::group_notifier &message_written_notifier,
    message_queue_t &message_queue,
    std::atomic<std::int32_t> &produced_message_count) noexcept
{
  message_t message;

  for (std::size_t i{0}; i < message_count; ++i)
  {
    const auto ret =
        snprintf(message.data(), message.size(),
                 "message with id: %zu from producer: %zu", i, info.group_id);

    if (unix::operation_failed(ret))
    {
      std::println("failed to create message");
      return false;
    }
    message_queue.push(message);
    produced_message_count++;

    std::println("message written into shared memeory by producer: {}",
                 info.group_id);
    const auto message_written = message_written_notifier.notify_one();

    if (!message_written)
    {
      std::println(
          "failed to send signal about message being written due to: {}",
          unix::to_string(message_written.error()).data());
      return false;
    }
    const auto message_read = message_read_notifier.wait_for_one();

    if (!message_read)
    {
      std::println(
          "failed to receive signal about message being read due to: {}",
          unix::to_string(message_read.error()).data());
      return false;
    }
  }
  return true;
}

bool wait_till_all_children_termninate() noexcept
{
  int status;

  while (true)
  {
    const auto child_terminated = unix::wait_till_child_terminates(&status);

    if (!child_terminated)
    {
      const auto error = child_terminated.error();

      if (error.code != ECHILD)
      {
        std::println("failed waiting for a child: {}",
                     unix::to_string(error).data());
        return false;
      }
      std::println("all children terminated");
      break;
    }
    const auto child_id = child_terminated.value();

    std::println("child with process id: {} terminated", child_id);

    if (WIFEXITED(status))
    {
      std::println("child exit status: {}", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
      psignal(WTERMSIG(status), "child exit signal");
    }
  }
  return true;
}

process_info create_child_processes(std::size_t child_producer_count,
                                    std::size_t child_consumer_count) noexcept
{
  constexpr std::size_t process_counts_size{2};
  using process_counts_t = std::array<std::size_t, process_counts_size>;
  static_assert(process_counts_size == std::size_t{2});
  const auto process_counts =
      process_counts_t{child_producer_count, child_consumer_count};
  process_info info;
  info.is_child = true;
  std::size_t i{0};
  assert(info.is_producer == false);

  for (std::size_t c{0}; c < process_counts.size(); ++c)
  {
    info.is_producer = c;

    for (i = 0; i < process_counts[c]; ++i)
    {
      const auto process_created = unix::create_process();

      if (!process_created)
      {
        std::println("failed to create child process due to: {}",
                     unix::to_string(process_created.error()).data());
        break;
      }
      info.created_process_count++;

      if (!unix::is_child_process(process_created.value()))
      {
        std::println("created child process with id: {}",
                     process_created.value());
      }
      else
      {
        return info;
      }
    }
  }
  info.is_child = false;
  info.is_producer = true;
  info.group_id = i;
  return info;
}

void *adavance_pointer(void *ptr, std::size_t byte_count) noexcept
{
  char *byte_ptr = static_cast<char *>(ptr);
  byte_ptr += byte_count;
  return static_cast<void *>(byte_ptr);
}

bool wait_till_production_complete(
    std::size_t producer_count,
    const unix::system_v::ipc::group_notifier &producers_notifier) noexcept
{
  const auto production_stopped = producers_notifier.wait_for_all();

  if (!production_stopped)
  {
    std::println(
        "failed to receive confirmation that production has stopped: {}",
        unix::to_string(production_stopped.error()).data());
    return false;
  }
  return true;
}

bool stop_consumption(
    const unix::system_v::ipc::group_notifier &message_written_notifier,
    std::atomic<bool> &done_flag) noexcept
{
  done_flag.store(true);
  const auto stop_consumption = message_written_notifier.notify_all();

  if (!stop_consumption)
  {
    std::println("failed to not notify consumers to stop due to: {}",
                 unix::to_string(stop_consumption.error()).data());
    return false;
  }
  return true;
}

bool start_production(
    const unix::system_v::ipc::group_notifier &producers_notifier) noexcept
{
  const auto start_production = producers_notifier.notify_all();

  if (!start_production)
  {
    std::println("failed to send start signal to producers");
    return false;
  }
  return true;
}

struct shared_data
{
  std::atomic<bool> done_flag{false};
  std::atomic<std::int32_t> produced_message_count{0};
  std::atomic<std::int32_t> consumed_message_count{0};
  message_queue_t message_queue;
};

int main(int, char **)
{
  using namespace unix::system_v;

  constexpr std::size_t semaphore_count{4}, readiness_sem_index{0},
      written_message_sem_index{1}, read_message_sem_index{2},
      producer_sem_index{3};
  constexpr std::size_t mem_size{sizeof(shared_data)}, create_process_count{10},
      message_count{200}, child_producer_count{2}, child_consumer_count{10},
      producer_count{child_producer_count + 1},
      consumer_count{child_consumer_count},
      children_count{child_producer_count + child_consumer_count};
  constexpr std::size_t producer_frequenecy{5};
  static_assert(create_process_count >= producer_frequenecy);

  constexpr auto perms = unix::permissions_builder{}
                             .owner_can_read()
                             .owner_can_write()
                             .owner_can_execute()
                             .group_can_read()
                             .group_can_write()
                             .group_can_execute()
                             .others_can_read()
                             .others_can_write()
                             .others_can_execute()
                             .get();
  auto shared_memory_created =
      ipc::shared_memory::create_private(mem_size, perms);

  if (!shared_memory_created)
  {
    std::println("failed to create shared memory due to: {}",
                 unix::to_string(shared_memory_created.error()).data());
    return EXIT_FAILURE;
  }
  std::println("shared memory created");
  auto &shared_memory = shared_memory_created.value();
  resource_remover_t<ipc::shared_memory> shared_memory_remover{&shared_memory};

  auto semaphore_created =
      ipc::semaphore_set::create_private(semaphore_count, perms);

  if (!semaphore_created)
  {
    std::println("failed to create semaphores");
    return EXIT_FAILURE;
  }
  std::println("semaphores created");
  auto &semaphores = semaphore_created.value();
  resource_remover_t<ipc::semaphore_set> semaphore_remover{&semaphores};

  const auto message_written_notifier = unix::system_v::ipc::group_notifier{
      semaphores, written_message_sem_index, consumer_count};
  const auto message_read_notifier = unix::system_v::ipc::group_notifier{
      semaphores, read_message_sem_index, producer_count};
  const auto children_readiness_notifier = unix::system_v::ipc::group_notifier{
      semaphores, readiness_sem_index, children_count};
  const auto producers_notifier = unix::system_v::ipc::group_notifier{
      semaphores, producer_sem_index, producer_count};

  std::array<unsigned short, semaphore_count> init_values = {
      0, 0, message_queue_t::capacity(), 0};
  const auto semaphore_initialized = semaphores.set_values(init_values);

  if (!semaphore_initialized)
  {
    std::println("failed to initialize semaphores");
    return EXIT_FAILURE;
  }
  std::println("semaphores initialized");
  const auto info =
      create_child_processes(child_producer_count, child_consumer_count);

  if (!info.is_child)
  {
    if (info.created_process_count != children_count)
    {
      std::println("failed to create all children process");
      return EXIT_FAILURE;
    }
    assert(consumer_count > 0 && producer_count > 0);
    assert(info.is_producer);
  }

  // the parent process shall do the clean-up
  if (info.is_child)
  {
    // release does not call deleter
    shared_memory_remover.release();
    semaphore_remover.release();
  }
  auto memory_attached = shared_memory.attach_anywhere(0);

  if (!memory_attached)
  {
    std::println("failed to attach shared memory due to: {}",
                 unix::to_string(memory_attached.error()).data());
    return EXIT_FAILURE;
  }
  auto &memory = memory_attached.value();
  std::println("attached to shared memory");

  auto *data = new (memory.get()) shared_data{};
  const auto process_id = unix::get_process_id();

  if (info.is_child)
  {
    if (!signal_readiness_to_parent(children_readiness_notifier))
    {
      return EXIT_FAILURE;
    }
    std::println("child process with id: {} is ready", process_id);
  }
  else
  {
    std::println("wait till all children process are ready");

    if (!wait_till_all_children_ready(info.created_process_count,
                                      children_readiness_notifier))
    {
      return EXIT_FAILURE;
    }
    std::println("all child processes are ready");
    std::println("start message production");

    if (!start_production(producers_notifier))
    {
      return EXIT_FAILURE;
    }
    std::println("message production started");
  }

  if (info.is_producer)
  {
    std::println("wait till production start");

    if (!wait_till_production_start(producers_notifier))
    {
      return EXIT_FAILURE;
    }
    std::println("production started");
    std::println("produce messages");

    if (!produce_messages(info, message_count, message_read_notifier,
                          message_written_notifier, data->message_queue,
                          data->produced_message_count))
    {
      return EXIT_FAILURE;
    }
    std::println("message production done");
    std::println("notify about production completion");

    if (!notify_about_production_completion(producers_notifier))
    {
      return EXIT_FAILURE;
    }
    std::println("notified about production completion");
  }
  else
  {
    std::println("consume messages");

    if (!consume_messages(info, message_written_notifier, message_read_notifier,
                          data->message_queue, process_id, data->done_flag,
                          data->consumed_message_count))
    {
      return EXIT_FAILURE;
    }
    std::println("message consumption done");
  }

  if (!info.is_child)
  {
    std::println("wait for all production to complete");

    if (!wait_till_production_complete(producer_count, producers_notifier))
    {
      return EXIT_FAILURE;
    }
    std::println("all producers done");
    std::println("stop message consumption");

    if (!stop_consumption(message_written_notifier, data->done_flag))
    {
      return EXIT_FAILURE;
    }
    std::println("message consumption stopped");
    std::println("consumed message count: {}", data->consumed_message_count.load());
    std::println("produced message count: {}", data->produced_message_count.load());
    std::println("wait for all children to terminate");

    if (!wait_till_all_children_termninate())
    {
      return EXIT_FAILURE;
    }
    std::println("all done");
  }
  return EXIT_SUCCESS;
}