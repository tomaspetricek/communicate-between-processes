#ifndef UNIX_POSIX_IPC_PRIMITIVE_HPP
#define UNIX_POSIX_IPC_PRIMITIVE_HPP

namespace unix::posix::ipc
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
} // namespace unix::posix::ipc

#endif // UNIX_POSIX_IPC_PRIMITIVE_HPP