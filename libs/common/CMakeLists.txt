add_library(common INTERFACE)

target_include_directories(common INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(common INTERFACE unix)