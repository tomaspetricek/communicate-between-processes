add_executable(create_language ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(create_language PRIVATE core)
target_include_directories(create_language PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)