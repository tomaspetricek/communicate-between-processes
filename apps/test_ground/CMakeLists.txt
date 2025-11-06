add_executable(test_ground ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(test_ground PRIVATE common)
target_include_directories(test_ground PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)