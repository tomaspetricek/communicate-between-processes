cmake_minimum_required(VERSION 3.31.0)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

project(communicate_between_processes VERSION 0.1.0 LANGUAGES C CXX)

add_executable(use_pipes use_pipes.cpp ${SOURCES})
add_executable(use_semaphores use_semaphores.cpp ${SOURCES})
add_executable(use_message_queues use_message_queues.cpp ${SOURCES})
add_executable(use_permissions use_permissions.cpp ${SOURCES})
add_executable(create_processes create_processes.cpp ${SOURCES})
add_executable(generate_key generate_key.cpp ${SOURCES})
add_executable(use_system_v_semaphores use_system_v_semaphores.cpp ${SOURCES})
add_executable(create_directory create_directory.cpp ${SOURCES})

