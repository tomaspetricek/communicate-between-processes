add_executable(threading ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(threading PRIVATE core)
target_include_directories(threading PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)