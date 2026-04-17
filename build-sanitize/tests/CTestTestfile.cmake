# CMake generated Testfile for 
# Source directory: /workspace/tests
# Build directory: /workspace/build-sanitize/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(string_utils "/workspace/build-sanitize/tests/test_all" "--string_utils")
set_tests_properties(string_utils PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/CMakeLists.txt;34;add_test;/workspace/tests/CMakeLists.txt;0;")
add_test(renderer "/workspace/build-sanitize/tests/test_all" "--renderer")
set_tests_properties(renderer PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/CMakeLists.txt;35;add_test;/workspace/tests/CMakeLists.txt;0;")
add_test(critical_fixes "/workspace/build-sanitize/tests/test_all")
set_tests_properties(critical_fixes PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/CMakeLists.txt;36;add_test;/workspace/tests/CMakeLists.txt;0;")
add_test(thread_safety "/workspace/build-sanitize/tests/test_all" "--thread_safety")
set_tests_properties(thread_safety PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/CMakeLists.txt;37;add_test;/workspace/tests/CMakeLists.txt;0;")
add_test(rect_safety "/workspace/build-sanitize/tests/test_all" "--rect_safety")
set_tests_properties(rect_safety PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/CMakeLists.txt;38;add_test;/workspace/tests/CMakeLists.txt;0;")
add_test(desktop_manager "/workspace/build-sanitize/tests/test_all" "--desktop_manager")
set_tests_properties(desktop_manager PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/CMakeLists.txt;39;add_test;/workspace/tests/CMakeLists.txt;0;")
add_test(all "/workspace/build-sanitize/tests/test_all")
set_tests_properties(all PROPERTIES  _BACKTRACE_TRIPLES "/workspace/tests/CMakeLists.txt;40;add_test;/workspace/tests/CMakeLists.txt;0;")
