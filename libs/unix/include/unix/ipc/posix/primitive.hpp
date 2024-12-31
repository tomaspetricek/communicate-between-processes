#ifndef UNIX_IPC_POSIX_PRIMITIVE_HPP
#define UNIX_IPC_POSIX_PRIMITIVE_HPP

namespace unix::ipc::posix
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
} // namespace unix::ipc::posix

#endif // UNIX_IPC_POSIX_PRIMITIVE_HPP