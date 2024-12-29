add_executable(sleeping_barber ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(sleeping_barber PRIVATE unix)
target_include_directories(sleeping_barber PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)