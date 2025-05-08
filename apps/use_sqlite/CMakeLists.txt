find_package(SQLite3 REQUIRED)

add_executable(use_sqlite ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(use_sqlite PRIVATE SQLite::SQLite3)
target_include_directories(use_sqlite PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)