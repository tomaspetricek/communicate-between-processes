#include <print>
#include <cstdlib>
#include <thread>
#include <chrono>

namespace console {
    bool clear() {
        const auto code = std::system("clear");
        return code != -1;
    }
}

int main() {
    std::println("clearing page");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    if (!console::clear()) {
        std::println("failed to clear console");
        return EXIT_FAILURE;
    }
    std::println("console cleared");
    return EXIT_SUCCESS;
}