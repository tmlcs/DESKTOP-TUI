# Unit tests CMakeLists.txt
cmake_minimum_required(VERSION 3.14)

add_executable(test_all
    test_string_utils.cpp
    test_renderer.cpp
)

target_include_directories(test_all PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME string_utils COMMAND test_all --string_utils)
add_test(NAME renderer COMMAND test_all --renderer)
add_test(NAME all COMMAND test_all)
