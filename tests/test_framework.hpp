#pragma once
#include <cstdio>

namespace tui {
    void run_string_utils_tests(int* passed, int* failed);
    void run_renderer_tests(int* passed, int* failed);
    int run_critical_fixes_main();
    void run_thread_safety_tests();
    void run_rect_safety_tests();
    void run_desktop_manager_tests();
    void run_braille_tests();
}
