cmake_minimum_required(VERSION 3.31.0)

project(communicate_between_processes VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# set highest speed optimizations
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

# add external libraries
add_subdirectory(external/strong_type)
add_subdirectory(external/googletest)
add_subdirectory(external/etl)

# enable strong type formatting 
add_compile_definitions(STRONG_HAS_STD_FORMAT=1)

# enable etl assertions
add_compile_definitions(ETL_DEBUG=1)

# add unit tests
enable_testing()
add_subdirectory(tests)

# add libraries
add_subdirectory(libs/core)
add_subdirectory(libs/common)
add_subdirectory(libs/disk_scheduling)
add_subdirectory(libs/lock_free)
add_subdirectory(libs/test)
add_subdirectory(libs/unix)

# add applications
add_subdirectory(apps/asynchronous_logger)
add_subdirectory(apps/consumer_producer_problem)
add_subdirectory(apps/create_language)
add_subdirectory(apps/create_logger)
add_subdirectory(apps/sleeping_barber_problem)

add_executable(use_pipes use_pipes.cpp)
target_link_libraries(use_pipes PRIVATE unix)

add_executable(use_semaphores use_semaphores.cpp)
target_link_libraries(use_semaphores PRIVATE unix)

add_executable(use_message_queues use_message_queues.cpp)
target_link_libraries(use_message_queues PRIVATE unix)

add_executable(use_permissions use_permissions.cpp)
target_link_libraries(use_permissions PRIVATE unix)

add_executable(create_processes create_processes.cpp)
target_link_libraries(create_processes PRIVATE unix)

add_executable(generate_key generate_key.cpp)
target_link_libraries(generate_key PRIVATE unix)

add_executable(use_system_v_semaphores use_system_v_semaphores.cpp)
target_link_libraries(use_system_v_semaphores PRIVATE unix)

add_executable(create_directory create_directory.cpp)
target_link_libraries(create_directory PRIVATE unix)

add_executable(use_strong_type use_strong_type.cpp)
target_link_libraries(use_strong_type PRIVATE strong_type)

add_executable(use_etl use_etl.cpp)
target_link_libraries(use_etl PRIVATE etl)

add_executable(use_scan use_scan.cpp)
target_link_libraries(use_scan PRIVATE disk_scheduling)