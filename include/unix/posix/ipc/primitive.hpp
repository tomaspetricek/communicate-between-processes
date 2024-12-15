#ifndef UNIX_POSIX_IPC_PRIMITIVE_HPP
#define UNIX_POSIX_IPC_PRIMITIVE_HPP


namespace unix::posix::ipc
{
    class primitive
    {
    public:
        explicit primitive() noexcept = default;
        
        primitive(const primitive &other) = delete;
        primitive &operator=(const primitive &other) = delete;

        primitive(primitive &&other) noexcept = delete;
        primitive &operator=(primitive &&other) noexcept = delete;
    };
}

#endif // UNIX_POSIX_IPC_PRIMITIVE_HPP