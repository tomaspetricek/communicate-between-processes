add_library(lock_free INTERFACE)

target_include_directories(lock_free INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(lock_free INTERFACE core)