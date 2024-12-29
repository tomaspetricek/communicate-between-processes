add_executable(consumer_producer ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(consumer_producer PRIVATE core unix)
target_include_directories(consumer_producer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)