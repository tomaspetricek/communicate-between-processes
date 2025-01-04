add_executable(async_logger ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(async_logger PRIVATE core lock_free unix)
target_include_directories(async_logger PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)