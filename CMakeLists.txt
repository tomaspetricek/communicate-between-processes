cmake_minimum_required(VERSION 3.31.0)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

project(communicate_between_processes VERSION 0.1.0 LANGUAGES C CXX)

# add libraries
add_subdirectory(libs/core)
add_subdirectory(libs/common)
add_subdirectory(libs/lock_free)
add_subdirectory(libs/unix)

# add applications
add_subdirectory(apps/consumer_producer_problem)
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