# Unit tests CMakeLists.txt
cmake_minimum_required(VERSION 3.14)

add_executable(test_all
    test_string_utils.cpp
    test_renderer.cpp
    test_critical_fixes.cpp
    test_thread_safety.cpp
    test_rect_safety.cpp
    test_desktop_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/desktop/desktop.cpp
    ${CMAKE_SOURCE_DIR}/src/desktop/desktop_manager.cpp
    ${CMAKE_SOURCE_DIR}/src/window/window.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/panel.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/renderer.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/text_input.cpp
)

target_include_directories(test_all PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Link pthread for thread safety tests
find_package(Threads REQUIRED)
target_link_libraries(test_all PRIVATE Threads::Threads)

add_test(NAME string_utils COMMAND test_all --string_utils)
add_test(NAME renderer COMMAND test_all --renderer)
add_test(NAME critical_fixes COMMAND test_all)
add_test(NAME thread_safety COMMAND test_all --thread_safety)
add_test(NAME rect_safety COMMAND test_all --rect_safety)
add_test(NAME desktop_manager COMMAND test_all --desktop_manager)
add_test(NAME all COMMAND test_all)

# Performance benchmarks (not run as tests — invoke manually)
add_executable(benchmark benchmark.cpp)
target_include_directories(benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(benchmark PRIVATE ${CMAKE_THREAD_LIBS_INIT})
