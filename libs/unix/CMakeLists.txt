add_library(unix INTERFACE)

target_include_directories(unix INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(common INTERFACE core)