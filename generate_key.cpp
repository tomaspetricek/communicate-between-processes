#include <print>

#include <unix/error_code.hpp>
#include <unix/system_v/ipc/key.hpp>

int main(int, char **)
{
    const auto key_generated = unix::system_v::ipc::generate_key("/does_not_exist.txt", 1);

    if (!key_generated)
    {
        std::println("failed to generate ipc key due to: {}", unix::to_string(key_generated.error()).data());
        return EXIT_FAILURE;
    }
    std::println("generated ipc key: {}", key_generated.value());
}