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
using message_queue_t = lock_free::ring_buffer<message_t, message_queue_capacity>;

// ToDo: remove global state
constexpr std::size_t semaphore_count{3}, readiness_sem_index{0},
    written_message_sem_index{1}, read_message_sem_index{2};

bool signal_readiness_to_parent(
    unix::system_v::ipc::semaphore_set &semaphores) noexcept
{
  const auto readiness_signaled =
      semaphores.increase_value(readiness_sem_index, 1);

  if (!readiness_signaled)
  {
    std::println("failed to send signal about readiness due to: {}",
                 unix::to_string(readiness_signaled.error()).data());
    return false;
  }
  return true;
}

bool process_messages(unix::system_v::ipc::semaphore_set &semaphores,
                      message_queue_t& message_queue,
                      const unix::process_id_t &process_id,
                      const std::atomic<bool> &done_flag) noexcept
{
  while (true)
  {
    std::println("wait for message");
    const auto message_written =
        semaphores.decrease_value(written_message_sem_index, -1);

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
    std::println("received message: {} by child with id: {}", message.data(),
                 process_id);
    const auto message_read =
        semaphores.increase_value(read_message_sem_index, 1);

    if (!message_read)
    {
      std::println("failed to send signal about message being read due to: {}",
                   unix::to_string(message_read.error()).data());
      return false;
    }
  }
  return true;
}

bool wait_till_all_children_ready(
    const std::size_t created_process_count,
    unix::system_v::ipc::semaphore_set &semaphores) noexcept
{
  using namespace unix::system_v;

  const auto readiness_signaled = semaphores.decrease_value(
      readiness_sem_index,
      -static_cast<ipc::semaphore_value_t>(created_process_count));

  if (!readiness_signaled)
  {
    std::println("signal about child being ready not received due to: {}",
                 unix::to_string(readiness_signaled.error()).data());
    return false;
  }
  return true;
}

bool generate_messages(const std::size_t created_process_count,
                       const std::size_t message_count,
                       unix::system_v::ipc::semaphore_set &semaphores,
                       message_queue_t& message_queue,
                       std::atomic<bool> &done_flag) noexcept
{
  for (std::size_t i{0}; i < message_count; ++i)
  {
    message_t message;
    const auto ret = snprintf(message.data(), message.size(), "message with id: %zu", i);

    if (unix::operation_failed(ret))
    {
      std::println("failed to create message");
      return false;
    }
    message_queue.push(std::move(message));

    std::println("message written into shared memeory");
    const auto message_written =
        semaphores.increase_value(written_message_sem_index, 1);

    if (!message_written)
    {
      std::println(
          "failed to send signal about message being written due to: {}",
          unix::to_string(message_written.error()).data());
      return false;
    }
    const auto message_read =
        semaphores.decrease_value(read_message_sem_index, -1);

    if (!message_read)
    {
      std::println(
          "failed to receive signal about message being read due to: {}",
          unix::to_string(message_read.error()).data());
      return false;
    }
  }
  std::println("generating done");
  done_flag.store(true);
  const auto generating_done = semaphores.increase_value(
      written_message_sem_index, created_process_count);

  if (!generating_done)
  {
    std::println("failed to send signal to children about message generating "
                 "being done: {}",
                 unix::to_string(generating_done.error()).data());
    return false;
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

struct process_info
{
  std::size_t created_process_count{0};
  bool is_child{false};
};

process_info create_child_processes(std::size_t create_process_count) noexcept
{
  process_info info;

  for (std::size_t i{0}; i < create_process_count; ++i)
  {
    const auto process_created = unix::create_process();

    if (!process_created)
    {
      std::println("failed to create child process due to: {}",
                   unix::to_string(process_created.error()).data());
      continue;
    }
    if (!unix::is_child_process(process_created.value()))
    {
      std::println("created child process with id: {}",
                   process_created.value());
    }
    else
    {
      info.is_child = true;
      break;
    }
    info.created_process_count++;
  }
  return info;
}

bool consume_messages(unix::system_v::ipc::semaphore_set &semaphores,
                      message_queue_t& message_queue,
                      const std::atomic<bool> &done_flag) noexcept
{
  const auto process_id = unix::get_process_id();

  if (!signal_readiness_to_parent(semaphores))
  {
    return false;
  }
  std::println("child process with id: {} is ready", process_id);

  if (!process_messages(semaphores, message_queue, process_id, done_flag))
  {
    return false;
  }
  return true;
}

bool produce_messages(std::size_t created_process_count,
                      std::size_t message_count,
                      unix::system_v::ipc::semaphore_set &semaphores,
                      message_queue_t& message_queue,
                      std::atomic<bool> &done_flag) noexcept
{
  std::println("wait till all children process are ready");

  if (!wait_till_all_children_ready(created_process_count, semaphores))
  {
    return false;
  }
  std::println("generate messeages");

  if (!generate_messages(created_process_count, message_count, semaphores,
                         message_queue, done_flag))
  {
    return false;
  }
  std::println("wait for all children to terminate");

  if (!wait_till_all_children_termninate())
  {
    return false;
  }
  std::println("all done");
  return true;
}

void *adavance_pointer(void *ptr, std::size_t byte_count) noexcept
{
  char *byte_ptr = static_cast<char *>(ptr);
  byte_ptr += byte_count;
  return static_cast<void *>(byte_ptr);
}

struct shared_data
{
  std::atomic<bool> done_flag{false};
  message_queue_t message_queue;
};

int main(int, char **)
{
  using namespace unix::system_v;

  constexpr std::size_t mem_size{sizeof(shared_data)}, create_process_count{10},
      message_count{200};

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

  std::array<unsigned short, semaphore_count> init_values = {0, 0, message_queue_t::capacity()};
  const auto semaphore_initialized = semaphores.set_values(init_values);

  if (!semaphore_initialized)
  {
    std::println("failed to initialize semaphores");
    return EXIT_FAILURE;
  }
  std::println("semaphores initialized");

  const auto info = create_child_processes(create_process_count);

  if (!info.is_child && (info.created_process_count == 0))
  {
    std::println("failed to create any child process");
    return EXIT_FAILURE;
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

  auto *data = reinterpret_cast<shared_data *>(memory.get());

  if (info.is_child)
  {
    if (!consume_messages(semaphores, data->message_queue, data->done_flag))
    {
      return EXIT_FAILURE;
    }
  }
  else
  {
    if (!produce_messages(info.created_process_count, message_count, semaphores,
                          data->message_queue, data->done_flag))
    {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}