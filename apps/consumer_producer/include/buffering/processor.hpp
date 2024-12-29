#ifndef BUFFERING_PROCESSOR_HPP
#define BUFFERING_PROCESSOR_HPP

#include <variant>

#include "unix/process.hpp"

#include "buffering/occupation.hpp"
#include "buffering/process_info.hpp"
#include "buffering/role.hpp"

namespace buffering
{
    class processor
    {
    public:
        explicit processor(const role_t &role,
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
        role_t role_;
        occupation_t occupation_;
    };

    processor create_processor(
        const process_info &info, unix::process_id_t process_id,
        std::size_t message_count,
        const unix::system_v::ipc::group_notifier &message_written_notifier,
        const unix::system_v::ipc::group_notifier &message_read_notifier,
        const unix::system_v::ipc::group_notifier &children_readiness_notifier,
        const unix::system_v::ipc::group_notifier &producers_notifier,
        std::atomic<bool> &done_flag,
        std::atomic<std::int32_t> &produced_message_count,
        std::atomic<std::int32_t> &consumed_message_count,
        buffering::message_queue_t &message_queue) noexcept
    {
        buffering::role_t role;

        if (info.is_child)
        {
            role = buffering::role::child{process_id, children_readiness_notifier};
        }
        else
        {
            role = buffering::role::parent{info,
                                           children_readiness_notifier,
                                           producers_notifier,
                                           message_written_notifier,
                                           consumed_message_count,
                                           produced_message_count,
                                           done_flag};
        }
        buffering::occupation_t occupation;

        if (info.is_producer)
        {
            occupation = buffering::occupation::producer{info,
                                                         message_count,
                                                         producers_notifier,
                                                         message_read_notifier,
                                                         message_written_notifier,
                                                         message_queue,
                                                         produced_message_count};
        }
        else
        {
            occupation = buffering::occupation::consumer{info,
                                                         process_id,
                                                         message_read_notifier,
                                                         message_written_notifier,
                                                         message_queue,
                                                         done_flag,
                                                         consumed_message_count};
        }
        return processor{role, occupation};
    }
} // namespace buffering

#endif // BUFFERING_PROCESSOR_HPP