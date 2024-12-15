#ifndef UNIX_SYSTEM_V_IPC_PRIMITIVE_HPP
#define UNIX_SYSTEM_V_IPC_PRIMITIVE_HPP

namespace unix::system_v::ipc
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

#endif // UNIX_SYSTEM_V_IPC_PRIMITIVE_HPP