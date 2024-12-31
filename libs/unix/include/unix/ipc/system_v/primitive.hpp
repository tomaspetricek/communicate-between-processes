#ifndef UNIX_IPC_SYSTEM_V_PRIMITIVE_HPP
#define UNIX_IPC_SYSTEM_V_PRIMITIVE_HPP

namespace unix::ipc::system_v
{
    class primitive
    {
    protected:
        explicit primitive() noexcept = default;

    public:
        primitive(const primitive &other) = delete;
        primitive &operator=(const primitive &other) = delete;

        primitive(primitive &&other) noexcept = delete;
        primitive &operator=(primitive &&other) noexcept = delete;
    };
}

#endif // UNIX_IPC_SYSTEM_V_PRIMITIVE_HPP