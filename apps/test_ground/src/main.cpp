#include <print>
#include <utility>
#include <expected>
#include <optional>
#include <cstdlib>
#include <variant>
#include <string>
#include <thread>
#include <chrono>

class playground_t
{
    using value_type = int;
    value_type value_{0};

    explicit playground_t(const value_type &val) : value_{val} {}

public:
    static std::optional<playground_t> create(const value_type &val)
    {
        if (val < 0)
        {
            return std::nullopt;
        }
        return playground_t(val);
    }

    const value_type &value() const
    {
        return value_;
    }
};

template <class First, class... Rest>
void print(const First &first, const Rest &...rest)
{
    constexpr auto rest_count = sizeof...(rest);
    std::println("{}: {}", rest_count, first);

    if constexpr (rest_count > 0)
    {
        print(rest...);
    }
}

class processor_base_t
{
public:
    virtual void process(const std::string_view &text) = 0;

    virtual ~processor_base_t() = default;
};

class faster_processor_t : public processor_base_t
{
public:
    void process(const std::string_view &text) override
    {
        std::println("processing: {}, A", text);
    }
};

class slower_processor_t : public processor_base_t
{
public:
    void process(const std::string_view &text) override
    {
        std::println("processing: {}, B", text);
    }
};

class move_only_t
{
public:
    explicit move_only_t() = default;

    // disable copying
    move_only_t(const move_only_t &src) = delete;
    move_only_t &operator=(const move_only_t &src) = delete;
};

template <class U, class T>
struct is_same
{
    static constexpr bool value = false;
};

template <class T>
struct is_same<T, T>
{
    static constexpr bool value = true;
};

template <class U, class T>
static constexpr auto is_same_v = is_same<U, T>::value;

void take_ownership(move_only_t &&object)
{
}

class base_t
{
public:
    base_t()
    {
        std::println("base constructor called");
    }

    ~base_t()
    {
        std::println("base destructor called");
    }
};

class derived_t : public base_t
{
public:
    derived_t()
    {
        std::println("derived constructed called");
    }

    ~derived_t()
    {
        std::println("derived destructor called");
    }
};

class csv_reader_t
{
public:
    template <class... Values>
    bool write_line(const Values &...vals)
    {
        constexpr std::size_t value_count = sizeof...(vals);
        return true;
    }
};

void keep_working(std::atomic<bool> &stop)
{
    while (!stop)
    {
        std::println("hello from a thread");
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int, char **)
{
    print(10, "text", 2.F);
    auto playground_created = playground_t::create(10);

    if (!playground_created.has_value())
    {
        std::println(stderr, "failed to create playground");
        return EXIT_FAILURE;
    }
    const auto &value = playground_created.value().value();
    std::println("playground value is: {}", value);

    std::variant<int, float, std::string> token;
    token = "hello world";

    std::visit([](const auto &val) -> void
               { std::println("token value is: {}", val); }, token);

    std::vector<std::unique_ptr<processor_base_t>> processors;
    processors.push_back(std::make_unique<faster_processor_t>());
    processors.push_back(std::make_unique<slower_processor_t>());

    for (const auto &processor : processors)
    {
        processor->process("text");
    }
    move_only_t object;
    take_ownership(std::move(object));

    static_assert(is_same_v<faster_processor_t, faster_processor_t>);

    {
        derived_t derived{};
    }
    constexpr std::size_t value_count{5};
    std::array<int, value_count> values{1, 2, 3, 4, 5};

    for (const auto &val : values)
    {
        std::print("{}, ", val);
    }
    std::println("");

    csv_reader_t reader;
    reader.write_line(10, "value");

    static_assert(std::is_same_v<int, std::decay_t<const int &>>);

    std::atomic<bool> stop{false};
    std::thread thread(keep_working, std::ref(stop));
    std::this_thread::sleep_for(std::chrono::seconds(10));
    stop = true;
    thread.join();
    return EXIT_SUCCESS;
}