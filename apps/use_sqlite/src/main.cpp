#include <cassert>
#include <cstdint>
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

class statement
{
    sqlite3_stmt *stmt_{nullptr};

    explicit statement(sqlite3_stmt *stmt) noexcept : stmt_{stmt} {}

public:
    // forbid copying
    statement(const statement &other) = delete;
    statement &operator=(const statement &other) = delete;

    // specify moving
    statement(statement &&other) noexcept
        : stmt_{std::exchange(other.stmt_, nullptr)} {}
    statement &operator=(statement &&other) noexcept
    {
        std::swap(stmt_, other.stmt_);
        return *this;
    }

    static std::optional<statement> create(sqlite3 *database_handle,
                                           const char *query) noexcept
    {
        assert(database_handle != nullptr);
        sqlite3_stmt *stmt{nullptr};

        if (sqlite3_prepare_v2(database_handle, query, -1, &stmt, NULL) !=
            SQLITE_OK)
        {
            std::println(stderr, "failed to prepare statement: {}",
                         sqlite3_errmsg(database_handle));
            return std::nullopt;
        }
        return statement{stmt};
    }

    bool set_value(int column, int value) noexcept
    {
        assert(column >= 1);
        if (sqlite3_bind_int(stmt_, column, value) != SQLITE_OK)
        {
            std::println(stderr, "failed to set value: {} to column: {}", value,
                         column);
            return false;
        }
        return true;
    }

    void get_value(int &value, int column) noexcept
    {
        assert(column >= 0);
        value = sqlite3_column_int(stmt_, 0);
    }

    ~statement() noexcept
    {
        if (stmt_ == nullptr)
        {
            return;
        }
        if (sqlite3_finalize(stmt_) != SQLITE_OK)
        {
            std::println(stderr, "failed to finalize statement");
        }
    }
};

class person_writer
{
    static constexpr std::string_view query =
        "INSERT INTO people (name, age) VALUES (?, ?);";
    statement stmt_;

    explicit person_writer(statement &&stmt) noexcept : stmt_{std::move(stmt)} {}

public:
    static std::optional<person_writer>
    create(sqlite3 *database_handle) noexcept
    {
        assert(database_handle != nullptr);
        auto stmt = statement::create(database_handle, query.data());

        if (!stmt.has_value())
        {
            std::println(stderr, "failed to create statement");
            return std::nullopt;
        }
        return person_writer(std::move(stmt.value()));
    }

    bool write(const person_t &person) { return true; }
};

class sql_database
{
    sqlite3 *disk_database_handle_{nullptr}, *in_memory_database_handle_{nullptr};
    person_writer person_writer_;

    explicit sql_database(sqlite3 *disk_database_handle_,
                          sqlite3 *in_memory_database_handle,
                          person_writer &&person_writer) noexcept
        : disk_database_handle_{disk_database_handle_},
          in_memory_database_handle_{in_memory_database_handle},
          person_writer_{std::move(person_writer)} {}

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
        if (sqlite3_open(":memory:", &in_memory_database_handle) != SQLITE_OK)
        {
            std::println(stderr, "failed to open in memory database due to: {}",
                         sqlite3_errmsg(in_memory_database_handle));
            sqlite3_close(disk_database_handle);
            sqlite3_close(in_memory_database_handle);
            return std::nullopt;
        }
        auto person_writer_created =
            person_writer::create(in_memory_database_handle);

        if (!person_writer_created.has_value())
        {
            std::println(stderr, "failed to create person writer");
            sqlite3_close(disk_database_handle);
            sqlite3_close(in_memory_database_handle);
            return std::nullopt;
        }
        return sql_database(disk_database_handle, in_memory_database_handle,
                            std::move(person_writer_created.value()));
    }

    bool load_into_memory() noexcept
    {
        if (!copy_content(disk_database_handle_, in_memory_database_handle_))
        {
            std::println(
                stderr,
                "failed to copy content of disk database to in memory database");
            return true;
        }
        return true;
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

int main(int, char **)
{
    std::println("use sqlite database");
    auto database = sql_database::open("database.db");

    if (!database.has_value())
    {
        std::println(stderr, "failed to open database");
        return EXIT_FAILURE;
    }
    if (!database.value().load_into_memory())
    {
        std::println(stderr, "failed to load data into memory");
        return EXIT_FAILURE;
    }
    if (!database.value().flush_to_disk())
    {
        std::println(stderr, "failed to flush data to disk");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}