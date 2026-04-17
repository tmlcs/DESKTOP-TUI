// Unified test runner
#include <cstdio>
#include <cstring>

namespace tui {
    void run_string_utils_tests(int* passed, int* failed);
    void run_renderer_tests(int* passed, int* failed);
    int run_critical_fixes_main();
    void run_thread_safety_tests();
    void run_rect_safety_tests();
    void run_desktop_manager_tests();
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--string_utils") == 0) {
            int p = 0, f = 0;
            tui::run_string_utils_tests(&p, &f);
            printf("\n=== Results: %d passed, %d failed ===\n", p, f);
            return f > 0 ? 1 : 0;
        }
        if (strcmp(argv[1], "--renderer") == 0) {
            int p = 0, f = 0;
            tui::run_renderer_tests(&p, &f);
            printf("\n=== Results: %d passed, %d failed ===\n", p, f);
            return f > 0 ? 1 : 0;
        }
        if (strcmp(argv[1], "--critical") == 0) {
            int f = tui::run_critical_fixes_main();
            return f > 0 ? 1 : 0;
        }
        if (strcmp(argv[1], "--thread_safety") == 0) {
            tui::run_thread_safety_tests();
            return 0;
        }
        if (strcmp(argv[1], "--rect_safety") == 0) {
            tui::run_rect_safety_tests();
            return 0;
        }
        if (strcmp(argv[1], "--desktop_manager") == 0) {
            tui::run_desktop_manager_tests();
            return 0;
        }
        printf("Unknown test suite: %s\n", argv[1]);
        return 1;
    }
    
    // Run all tests
    printf("=== Running All Tests ===\n\n");
    
    int total_passed = 0, total_failed = 0;
    
    printf("--- String Utils ---\n");
    tui::run_string_utils_tests(&total_passed, &total_failed);
    
    printf("\n--- Renderer ---\n");
    tui::run_renderer_tests(&total_passed, &total_failed);
    
    printf("\n--- Critical Fixes ---\n");
    total_failed += tui::run_critical_fixes_main();
    
    printf("\n--- Thread Safety ---\n");
    tui::run_thread_safety_tests();
    
    printf("\n--- Rect Safety ---\n");
    tui::run_rect_safety_tests();
    
    printf("\n--- Desktop Manager ---\n");
    tui::run_desktop_manager_tests();
    
    printf("\n=== Final Results: %d passed, %d failed ===\n", total_passed, total_failed);
    return total_failed > 0 ? 1 : 0;
}
