add_executable(text_editor ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(text_editor PRIVATE core)
target_include_directories(text_editor PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)