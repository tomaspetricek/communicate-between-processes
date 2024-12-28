#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <functional>
#include <memory>
#include <print>
#include <span>
#include <string_view>
#include <variant>

#include "lock_free/ring_buffer.hpp"
#include "string_literal.hpp"
#include "unix/error_code.hpp"
#include "unix/permissions_builder.hpp"
#include "unix/process.hpp"
#include "unix/signal.hpp"
#include "unix/system_v/ipc/group_notifier.hpp"
#include "unix/system_v/ipc/semaphore_set.hpp"
#include "unix/system_v/ipc/shared_memory.hpp"

template <string_literal ResourceName, class Resource>
static void remove_resource(Resource *resource) noexcept
{
  const auto removed = resource->remove();
  std::println("{} removed: {}", ResourceName.data(), removed.has_value());
  assert(removed);
}

template <string_literal ResourceName, class Resource>
static void destroy_resource(Resource *resource) noexcept
{
  resource->~Resource();
  std::println("{} destroyed", ResourceName.data());
}

template <string_literal ResourceName, class Resource>
using resource_remover_t =
    std::unique_ptr<Resource,
                    unix::deleter<remove_resource<ResourceName, Resource>>>;

template <string_literal ResourceName, class Resource>
using resource_destroyer_t =
    std::unique_ptr<Resource,
                    unix::deleter<destroy_resource<ResourceName, Resource>>>;

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
    std::println("failed to send start signal to producers due to: {}",
                 unix::to_string(start_production.error()).data());
    return false;
  }
  return true;
}

bool wait_till_all_messages_consumed(const unix::system_v::ipc::group_notifier
                                         &message_written_notifier) noexcept
{
  const auto all_consumed = message_written_notifier.wait_till_none();

  if (!all_consumed)
  {
    std::println("failed waiting for all messages to be consumed due to: {}",
                 unix::to_string(all_consumed.error()).data());
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

bool run_producer(
    const process_info &info, std::size_t message_count,
    const unix::system_v::ipc::group_notifier &producers_notifier,
    const unix::system_v::ipc::group_notifier &message_read_notifier,
    const unix::system_v::ipc::group_notifier &message_written_notifier,
    message_queue_t &message_queue,
    std::atomic<std::int32_t> &produced_message_count) noexcept
{
  std::println("wait till production start");

  if (!wait_till_production_start(producers_notifier))
  {
    return EXIT_FAILURE;
  }
  std::println("production started");
  std::println("produce messages");

  if (!produce_messages(info, message_count, message_read_notifier,
                        message_written_notifier, message_queue,
                        produced_message_count))
  {
    return false;
  }
  std::println("message production done");
  std::println("notify about production completion");

  if (!notify_about_production_completion(producers_notifier))
  {
    return false;
  }
  std::println("notified about production completion");
  return true;
}

bool run_consumer(
    const process_info &info, unix::process_id_t process_id,
    const unix::system_v::ipc::group_notifier &message_read_notifier,
    const unix::system_v::ipc::group_notifier &message_written_notifier,
    message_queue_t &message_queue, std::atomic<bool> &done_flag,
    std::atomic<std::int32_t> &consumed_message_count)
{
  std::println("consume messages");

  if (!consume_messages(info, message_written_notifier, message_read_notifier,
                        message_queue, process_id, done_flag,
                        consumed_message_count))
  {
    return false;
  }
  std::println("message consumption done");
  return true;
}

bool setup_child_process(unix::process_id_t process_id,
                         const unix::system_v::ipc::group_notifier
                             &children_readiness_notifier) noexcept
{
  if (!signal_readiness_to_parent(children_readiness_notifier))
  {
    return false;
  }
  std::println("child process with id: {} is ready", process_id);
  return true;
}

bool setup_parent_process(
    const process_info &info,
    const unix::system_v::ipc::group_notifier &children_readiness_notifier,
    const unix::system_v::ipc::group_notifier &producers_notifier) noexcept
{
  std::println("wait till all children process are ready");

  if (!wait_till_all_children_ready(info.created_process_count,
                                    children_readiness_notifier))
  {
    return false;
  }
  std::println("all child processes are ready");
  std::println("start message production");

  if (!start_production(producers_notifier))
  {
    return false;
  }
  std::println("message production started");
  return true;
}

bool finalize_parent_process(
    const unix::system_v::ipc::group_notifier &producers_notifier,
    const unix::system_v::ipc::group_notifier &message_written_notifier,
    const std::atomic<std::int32_t> &consumed_message_count,
    const std::atomic<std::int32_t> &produced_message_count,
    std::atomic<bool> &done_flag) noexcept
{
  std::println("wait for all production to complete");

  if (!wait_till_production_complete(producers_notifier))
  {
    return EXIT_FAILURE;
  }
  std::println("all producers done");
  std::println("wait till all messages consumed");

  if (!wait_till_all_messages_consumed(message_written_notifier))
  {
    return EXIT_FAILURE;
  }
  std::println("all messages consumed");
  std::println("consumed message count: {}",
               consumed_message_count.load(std::memory_order_relaxed));
  std::println("produced message count: {}",
               produced_message_count.load(std::memory_order_relaxed));
  assert(consumed_message_count.load() == produced_message_count.load());
  std::println("stop message consumption");

  if (!stop_consumption(message_written_notifier, done_flag))
  {
    return false;
  }
  std::println("message consumption stopped");
  std::println("wait for all children to terminate");

  if (!wait_till_all_children_termninate())
  {
    return false;
  }
  std::println("all done");
  return true;
}

class child
{
public:
  explicit child(unix::process_id_t process_id,
                 const unix::system_v::ipc::group_notifier
                     &children_readiness_notifier) noexcept
      : process_id_{process_id},
        children_readiness_notifier_{std::cref(children_readiness_notifier)} {}

  bool setup() const noexcept
  {
    return setup_child_process(process_id_, children_readiness_notifier_);
  }

  bool finalize() const noexcept { return true; }

private:
  unix::process_id_t process_id_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      children_readiness_notifier_;
};

struct no_role
{
  bool setup() const noexcept { return true; }
  bool finalize() noexcept { return true; }
};

class parent
{
public:
  explicit parent(
      const process_info &info,
      const unix::system_v::ipc::group_notifier &children_readiness_notifier,
      const unix::system_v::ipc::group_notifier &producers_notifier,
      const unix::system_v::ipc::group_notifier &message_written_notifier,
      const std::atomic<std::int32_t> &consumed_message_count,
      const std::atomic<std::int32_t> &produced_message_count,
      std::atomic<bool> &done_flag) noexcept
      : info_{info},
        children_readiness_notifier_{std::cref(children_readiness_notifier)},
        producers_notifier_{std::cref(producers_notifier)},
        message_written_notifier_{std::cref(message_written_notifier)},
        consumed_message_count_{std::cref(consumed_message_count)},
        produced_message_count_{std::cref(produced_message_count)},
        done_flag_{std::ref(done_flag)} {}

  bool setup() const noexcept
  {
    return setup_parent_process(info_, children_readiness_notifier_,
                                producers_notifier_);
  }

  bool finalize() noexcept
  {
    return finalize_parent_process(
        producers_notifier_, message_written_notifier_, consumed_message_count_,
        produced_message_count_, done_flag_);
  }

private:
  process_info info_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      children_readiness_notifier_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      producers_notifier_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      message_written_notifier_;
  std::reference_wrapper<const std::atomic<std::int32_t>>
      consumed_message_count_;
  std::reference_wrapper<const std::atomic<std::int32_t>>
      produced_message_count_;
  std::reference_wrapper<std::atomic<bool>> done_flag_;
};

using social_role_t = std::variant<no_role, child, parent>;

struct no_occupation
{
  bool run() noexcept { return true; }
};

class producer
{
public:
  explicit producer(
      const process_info &info, std::size_t message_count,
      const unix::system_v::ipc::group_notifier &producers_notifier,
      const unix::system_v::ipc::group_notifier &message_read_notifier,
      const unix::system_v::ipc::group_notifier &message_written_notifier,
      message_queue_t &message_queue,
      std::atomic<std::int32_t> &produced_message_count) noexcept
      : info_{info}, message_count_{message_count},
        producers_notifier_{producers_notifier},
        message_read_notifier_{message_read_notifier},
        message_written_notifier_{message_written_notifier},
        message_queue_{message_queue},
        produced_message_count_{produced_message_count} {}

  bool run() noexcept
  {
    return run_producer(info_, message_count_, producers_notifier_,
                        message_read_notifier_, message_written_notifier_,
                        message_queue_, produced_message_count_);
  }

private:
  process_info info_;
  std::size_t message_count_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      producers_notifier_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      message_read_notifier_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      message_written_notifier_;
  std::reference_wrapper<message_queue_t> message_queue_;
  std::reference_wrapper<std::atomic<std::int32_t>> produced_message_count_;
};

class consumer
{
public:
  explicit consumer(
      const process_info &info, unix::process_id_t process_id,
      const unix::system_v::ipc::group_notifier &message_read_notifier,
      const unix::system_v::ipc::group_notifier &message_written_notifier,
      message_queue_t &message_queue, std::atomic<bool> &done_flag,
      std::atomic<std::int32_t> &consumed_message_count) noexcept
      : info_{info}, process_id_{process_id},
        message_read_notifier_{message_read_notifier},
        message_written_notifier_{message_written_notifier},
        message_queue_{message_queue}, done_flag_{done_flag},
        consumed_message_count_{consumed_message_count} {}

  bool run() noexcept
  {
    return run_consumer(info_, process_id_, message_read_notifier_,
                        message_written_notifier_, message_queue_, done_flag_,
                        consumed_message_count_);
  }

private:
  process_info info_;
  unix::process_id_t process_id_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      message_read_notifier_;
  std::reference_wrapper<const unix::system_v::ipc::group_notifier>
      message_written_notifier_;
  std::reference_wrapper<message_queue_t> message_queue_;
  std::reference_wrapper<std::atomic<bool>> done_flag_;
  std::reference_wrapper<std::atomic<std::int32_t>> consumed_message_count_;
};

using occupation_t = std::variant<no_occupation, producer, consumer>;

class processor
{
public:
  explicit processor(const social_role_t &role,
                     const occupation_t &occupation) noexcept
      : role_{role}, occupation_{occupation} {}

  bool process() noexcept
  {
    if (!std::visit([](const auto &r)
                    { return r.setup(); }, role_))
    {
      return false;
    }
    if (!std::visit([](auto &o)
                    { return o.run(); }, occupation_))
    {
      return false;
    }
    if (!std::visit([](auto &r)
                    { return r.finalize(); }, role_))
    {
      return false;
    }
    return true;
  }

private:
  social_role_t role_;
  occupation_t occupation_;
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
  resource_remover_t<string_literal{"shared memory"}, ipc::shared_memory>
      shared_memory_remover{&shared_memory};

  auto semaphore_created =
      ipc::semaphore_set::create_private(semaphore_count, perms);

  if (!semaphore_created)
  {
    std::println("failed to create semaphores");
    return EXIT_FAILURE;
  }
  std::println("semaphores created");
  auto &semaphores = semaphore_created.value();
  resource_remover_t<string_literal{"semaphore set"}, ipc::semaphore_set>
      semaphore_remover{&semaphores};

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
  resource_destroyer_t<string_literal{"data"}, shared_data> data_destroyer{
      data};

  if (info.is_child)
  {
    data_destroyer.release();
  }
  const auto process_id = unix::get_process_id();

  social_role_t role;

  if (info.is_child)
  {
    role = child{process_id, children_readiness_notifier};
  }
  else
  {
    role = parent{info,
                  children_readiness_notifier,
                  producers_notifier,
                  message_written_notifier,
                  data->consumed_message_count,
                  data->produced_message_count,
                  data->done_flag};
  }
  occupation_t occupation;

  if (info.is_producer)
  {
    occupation = producer{info,
                          message_count,
                          producers_notifier,
                          message_read_notifier,
                          message_written_notifier,
                          data->message_queue,
                          data->produced_message_count};
  }
  else
  {
    occupation = consumer{info,
                          process_id,
                          message_read_notifier,
                          message_written_notifier,
                          data->message_queue,
                          data->done_flag,
                          data->consumed_message_count};
  }
  if (processor{role, occupation}.process())
  {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}