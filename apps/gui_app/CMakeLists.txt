find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

add_executable(gui_app MACOSX_BUNDLE ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp)
target_link_libraries(gui_app PRIVATE core etl)
target_include_directories(gui_app PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)