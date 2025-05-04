add_executable(create_logger ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(create_logger PRIVATE core etl)
target_include_directories(create_logger PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)