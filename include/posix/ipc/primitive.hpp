#ifndef POSIX_IPC_PRIMITIVE_HPP
#define POSIX_IPC_PRIMITIVE_HPP


namespace posix::ipc
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

#endif // POSIX_IPC_PRIMITIVE_HPP