cmake_minimum_required(VERSION 3.31.0)

set(CMAKE_CXX_STANDARD 23)

include_directories(include)

file(GLOB SOURCES "src/*.cpp")

project(communicate_between_processes VERSION 0.1.0 LANGUAGES C CXX)

add_executable(use_pipes use_pipes.cpp ${SOURCES})
add_executable(use_semaphores use_semaphores.cpp ${SOURCES})
add_executable(use_message_queues use_message_queues.cpp ${SOURCES})

