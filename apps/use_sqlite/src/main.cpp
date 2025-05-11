#include <cassert>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <print>
#include <sqlite3.h>
#include <string>
#include <string_view>
#include <vector>

bool copy_content(sqlite3 *src_handle, sqlite3 *dest_handle) noexcept
{
    assert(src_handle != nullptr);
    assert(dest_handle != nullptr);

    sqlite3_backup *backup =
        sqlite3_backup_init(dest_handle, "main", src_handle, "main");

    if (backup == nullptr)
    {
        std::println(stderr, "failed to initialize backup due to: {}",
                     sqlite3_errmsg(dest_handle));
        return false;
    }
    if (sqlite3_backup_step(backup, -1) != SQLITE_DONE)
    {
        std::println(stderr, "failed to perform backup due to: {}",
                     sqlite3_errmsg(dest_handle));
        return false;
    }
    if (sqlite3_backup_finish(backup) != SQLITE_OK)
    {
        std::println(stderr, "failed to finish backup due to: {}",
                     sqlite3_errmsg(dest_handle));
        return false;
    }
    return true;
}

struct person_t
{
    std::string name;
    std::int32_t age{0};
};

class prepared_statement
{
    sqlite3 *database_handle_{nullptr};
    sqlite3_stmt *stmt_{nullptr};

    explicit prepared_statement(sqlite3 *database_handle,
                                sqlite3_stmt *stmt) noexcept
        : database_handle_{database_handle}, stmt_{stmt} {}

public:
    // forbid copying
    prepared_statement(const prepared_statement &other) = delete;
    prepared_statement &operator=(const prepared_statement &other) = delete;

    // specify moving
    prepared_statement(prepared_statement &&other) noexcept
        : database_handle_{std::exchange(other.database_handle_, nullptr)},
          stmt_{std::exchange(other.stmt_, nullptr)} {}
    prepared_statement &operator=(prepared_statement &&other) noexcept
    {
        std::swap(database_handle_, other.database_handle_);
        std::swap(stmt_, other.stmt_);
        return *this;
    }

    static std::optional<prepared_statement> create(sqlite3 *database_handle,
                                                    const char *query) noexcept
    {
        assert(database_handle != nullptr);
        assert(query != nullptr);
        sqlite3_stmt *stmt{nullptr};

        if (sqlite3_prepare_v2(database_handle, query, -1, &stmt, NULL) !=
            SQLITE_OK)
        {
            std::println(stderr, "failed to prepare statement: {}",
                         sqlite3_errmsg(database_handle));
            return std::nullopt;
        }
        return prepared_statement{database_handle, stmt};
    }

    bool set_value(int index, int value) noexcept
    {
        assert(database_handle_ != nullptr);
        assert(stmt_ != nullptr);
        assert(index >= 0);

        if (sqlite3_bind_int(stmt_, index + 1, value) != SQLITE_OK)
        {
            std::println(stderr, "failed to set value: {} at index: {}", value,
                         index);
            return false;
        }
        return true;
    }

    bool set_value(int index, const std::string_view &value) noexcept
    {
        assert(database_handle_ != nullptr);
        assert(stmt_ != nullptr);

        if (sqlite3_bind_text(stmt_, index + 1, value.data(), -1,
                              SQLITE_TRANSIENT) != SQLITE_OK)
        {
            std::println(stderr, "failed to set value: {} at index: {}", value.data(),
                         index);
            return false;
        }
        return true;
    }

    std::optional<int> execute() noexcept
    {
        assert(database_handle_ != nullptr);
        assert(stmt_ != nullptr);

        const auto code = sqlite3_step(stmt_);

        if (code != SQLITE_DONE && code != SQLITE_ROW)
        {
            std::println(stderr, "failed to execute statement due to: {}",
                         sqlite3_errmsg(database_handle_));
            return std::nullopt;
        }
        return code;
    }

    bool reset() noexcept
    {
        assert(database_handle_ != nullptr);
        assert(stmt_ != nullptr);

        if (sqlite3_reset(stmt_) != SQLITE_OK)
        {
            std::println(stderr, "failed to reset prepared statement due to: {}",
                         sqlite3_errmsg(database_handle_));
            return false;
        }
        return true;
    }

    void get_value(int &value, int column) noexcept
    {
        assert(database_handle_ != nullptr);
        assert(stmt_ != nullptr);
        assert(column >= 0);
        value = sqlite3_column_int(stmt_, 0);
    }

    bool destroy() noexcept
    {
        if (stmt_ == nullptr)
        {
            return true;
        }
        assert(database_handle_ != nullptr);
        assert(stmt_ != nullptr);

        if (sqlite3_finalize(stmt_) != SQLITE_OK)
        {
            std::println(stderr, "failed to finalize statement");
            return false;
        }
        stmt_ = nullptr;
        return true;
    }

    ~prepared_statement() noexcept { assert(destroy()); }
};

class person_writer
{
    static constexpr std::string_view query =
        "INSERT INTO people (name, age) VALUES (?, ?);";
    prepared_statement stmt_;

    explicit person_writer(prepared_statement &&stmt) noexcept
        : stmt_{std::move(stmt)} {}

public:
    static std::optional<person_writer>
    create(sqlite3 *database_handle) noexcept
    {
        assert(database_handle != nullptr);
        auto stmt = prepared_statement::create(database_handle, query.data());

        if (!stmt.has_value())
        {
            std::println(stderr, "failed to create prepared statement");
            return std::nullopt;
        }
        return person_writer(std::move(stmt.value()));
    }

    bool write(const person_t &person) noexcept
    {
        int index{0};

        if (!stmt_.set_value(index++, person.age))
        {
            std::println("failed to set age: {}", person.age);
            return false;
        }
        std::println("age value set to: {}", person.age);

        if (!stmt_.set_value(index++, person.name.data()))
        {
            std::println("failed to set name: {}", person.name);
            return false;
        }
        std::println("name value set to: {}", person.name.data());

        const auto executed = stmt_.execute();

        if (!executed)
        {
            std::println(
                "failed to execute statement: {} with value: age: {}, name: {}",
                query, person.age, person.name);
            return false;
        }
        std::println("prepare statement executed");
        assert(executed.value() == SQLITE_DONE);

        if (!stmt_.reset())
        {
            std::println(stderr,
                         "failed to ready prepare statement for next execution");
        }
        return true;
    }

    bool destroy() noexcept { return stmt_.destroy(); }
};

bool execute_script(sqlite3 *database_handle,
                    const std::string_view &script) noexcept
{
    assert(database_handle != nullptr);
    char *error_message = 0;

    if (sqlite3_exec(database_handle, script.data(), 0, 0, &error_message) !=
        SQLITE_OK)
    {
        std::println(stderr, "failed to run script: \ndue to: {}", script.data(),
                     error_message);
        sqlite3_free(error_message);
        return false;
    }
    return true;
}

class sql_database
{
    person_writer person_writer_;
    sqlite3 *disk_database_handle_{nullptr}, *in_memory_database_handle_{nullptr};

    explicit sql_database(sqlite3 *disk_database_handle_,
                          sqlite3 *in_memory_database_handle,
                          person_writer &&person_writer) noexcept
        : disk_database_handle_{disk_database_handle_},
          in_memory_database_handle_{in_memory_database_handle},
          person_writer_{std::move(person_writer)} {}

    static bool
    load_into_memory_impl(sqlite3 *disk_database_handle,
                          sqlite3 *in_memory_database_handle) noexcept
    {
        assert(disk_database_handle != nullptr);
        assert(in_memory_database_handle != nullptr);

        if (!copy_content(disk_database_handle, in_memory_database_handle))
        {
            std::println(
                stderr,
                "failed to copy content of disk database to in memory database");
            return true;
        }
        return true;
    }

public:
    // forbid copying
    sql_database(const sql_database &other) = delete;
    sql_database &operator=(const sql_database &other) = delete;

    // specify moving
    sql_database(sql_database &&other) noexcept
        : disk_database_handle_{std::exchange(other.disk_database_handle_,
                                              nullptr)},
          in_memory_database_handle_{
              std::exchange(other.in_memory_database_handle_, nullptr)},
          person_writer_{std::move(other.person_writer_)} {}
    sql_database &operator=(sql_database &&other) noexcept
    {
        std::swap(disk_database_handle_, other.disk_database_handle_);
        std::swap(in_memory_database_handle_, other.in_memory_database_handle_);
        person_writer_ = std::move(other.person_writer_);
        return *this;
    }

    static std::optional<sql_database>
    open(const std::string_view &database_path) noexcept
    {
        sqlite3 *disk_database_handle{nullptr}, *in_memory_database_handle{nullptr};

        if (sqlite3_open(database_path.data(), &disk_database_handle) !=
            SQLITE_OK)
        {
            std::println(stderr, "failed to open on disk database due to: {}",
                         sqlite3_errmsg(disk_database_handle));
            sqlite3_close(disk_database_handle);
            return std::nullopt;
        }
        std::println("disk database opened");

        if (sqlite3_open(":memory:", &in_memory_database_handle) != SQLITE_OK)
        {
            std::println(stderr, "failed to open in memory database due to: {}",
                         sqlite3_errmsg(in_memory_database_handle));
            sqlite3_close(disk_database_handle);
            sqlite3_close(in_memory_database_handle);
            return std::nullopt;
        }
        std::println("in memory database opened");
        std::println("loading content of disk database into in memory database");

        if (!load_into_memory_impl(disk_database_handle,
                                   in_memory_database_handle))
        {
            return std::nullopt;
        }
        std::println("content loaded into in memory database");

        auto person_writer_created =
            person_writer::create(in_memory_database_handle);

        if (!person_writer_created.has_value())
        {
            std::println(stderr, "failed to create person writer");
            sqlite3_close(disk_database_handle);
            sqlite3_close(in_memory_database_handle);
            return std::nullopt;
        }
        std::println("person writer created");
        return sql_database(disk_database_handle, in_memory_database_handle,
                            std::move(person_writer_created.value()));
    }

    bool load_into_memory() noexcept
    {
        return load_into_memory_impl(disk_database_handle_,
                                     in_memory_database_handle_);
    }

    bool flush_to_disk() const
    {
        if (!copy_content(in_memory_database_handle_, disk_database_handle_))
        {
            std::println(
                stderr,
                "failed to copy content of in memory database to disk database");
            return false;
        }
        if (sqlite3_db_cacheflush(disk_database_handle_) != SQLITE_OK)
        {
            std::println(stderr, "failed to flush cache due to: {}",
                         sqlite3_errmsg(disk_database_handle_));
            return false;
        }
        return true;
    }

    bool write_person(const person_t &person) noexcept
    {
        return person_writer_.write(person);
    }

    ~sql_database() noexcept
    {
        person_writer_.destroy();

        if (disk_database_handle_ == nullptr)
        {
            assert(in_memory_database_handle_ == nullptr);
            return;
        }
        if (sqlite3_close(disk_database_handle_) != SQLITE_OK)
        {
            std::println(stderr, "failed to close disk database due to: {}",
                         sqlite3_errmsg(disk_database_handle_));
        }
        if (sqlite3_close(in_memory_database_handle_) != SQLITE_OK)
        {
            std::println(stderr, "failed to close in memory database due to: {}",
                         sqlite3_errmsg(in_memory_database_handle_));
        }
    }
};

static constexpr std::string_view create_script{
    "CREATE TABLE IF NOT EXISTS people ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL,"
    "age INTEGER NOT NULL);"};

bool close_database(sqlite3 *database_handle,
                    const std::filesystem::path &database_path) noexcept
{
    assert(database_handle != nullptr);

    if (sqlite3_close(database_handle) != SQLITE_OK)
    {
        std::println(stderr, "failed to close database: {}",
                     database_path.string().data());
        return false;
    }
    return true;
}

bool create_database(const std::filesystem::path &destination,
                     const std::string_view &create_script) noexcept
{
    sqlite3 *database_handle{nullptr};

    if (sqlite3_open(destination.string().data(), &database_handle) !=
        SQLITE_OK)
    {
        std::println(stderr, "failed to open database: {}",
                     sqlite3_errmsg(database_handle));
        close_database(database_handle, destination);
        return false;
    }
    std::println("executing create script: \n{}", create_script.data());

    if (!execute_script(database_handle, create_script))
    {
        std::println(stderr, "failed to execute create script");
        close_database(database_handle, destination);
        return false;
    }
    close_database(database_handle, destination);
    return true;
}

int main(int, char **)
{
    std::println("use sqlite database");

    std::error_code error_code;
    const auto current_path = std::filesystem::current_path(error_code);

    if (error_code)
    {
        std::println(stderr, "failed to get current path due to: {}",
                     error_code.message());
        return EXIT_FAILURE;
    }
    std::println("current path {}", current_path.string().data());
    const auto database_path = current_path / "database.db";

    if (!create_database(database_path, create_script))
    {
        std::println("failed to create database: {}",
                     database_path.string().data());
        return EXIT_FAILURE;
    }
    std::println("database created: {}", database_path.string().data());

    auto database = sql_database::open(database_path.string().data());

    if (!database.has_value())
    {
        std::println(stderr, "failed to open database");
        return EXIT_FAILURE;
    }
    const person_t person{"Tom", 26};

    if (!database.value().write_person(person))
    {
        std::println(stderr, "failed to write person: name: {}, age: {}",
                     person.name, person.age);
        return EXIT_FAILURE;
    }
    std::println("person written");

    if (!database.value().flush_to_disk())
    {
        std::println(stderr, "failed to flush data to disk");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}