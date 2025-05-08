#include <print>
#include <sqlite3.h>

int main(int, char **)
{
    std::println("use sqlite database");

    sqlite3 *db{nullptr};
    if (sqlite3_open(":memory:", &db) == SQLITE_OK)
    {
        std::println("SQLite opened in memory");
        sqlite3_close(db);
    }
    else
    {
        std::println("failed to open SQLite DB\n");
    }
    return 0;
}