add_library(disk_scheduling INTERFACE)

target_include_directories(disk_scheduling INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(disk_scheduling INTERFACE etl)