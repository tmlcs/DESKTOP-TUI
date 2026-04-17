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
    void run_rect_tests(int* passed, int* failed);
    void run_colors_tests(int* passed, int* failed);
    void run_signal_tests(int* passed, int* failed);
    void run_event_tests(int* passed, int* failed);
    void run_widget_tests(int* passed, int* failed);
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
        if (strcmp(argv[1], "--rect") == 0) {
            int p = 0, f = 0;
            tui::run_rect_tests(&p, &f);
            printf("\n=== Results: %d passed, %d failed ===\n", p, f);
            return f > 0 ? 1 : 0;
        }
        if (strcmp(argv[1], "--colors") == 0) {
            int p = 0, f = 0;
            tui::run_colors_tests(&p, &f);
            printf("\n=== Results: %d passed, %d failed ===\n", p, f);
            return f > 0 ? 1 : 0;
        }
        if (strcmp(argv[1], "--signal") == 0) {
            int p = 0, f = 0;
            tui::run_signal_tests(&p, &f);
            printf("\n=== Results: %d passed, %d failed ===\n", p, f);
            return f > 0 ? 1 : 0;
        }
        if (strcmp(argv[1], "--event") == 0) {
            int p = 0, f = 0;
            tui::run_event_tests(&p, &f);
            printf("\n=== Results: %d passed, %d failed ===\n", p, f);
            return f > 0 ? 1 : 0;
        }
        if (strcmp(argv[1], "--widgets") == 0) {
            int p = 0, f = 0;
            tui::run_widget_tests(&p, &f);
            printf("\n=== Results: %d passed, %d failed ===\n", p, f);
            return f > 0 ? 1 : 0;
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
    
    printf("\n--- Rect ---\n");
    tui::run_rect_tests(&total_passed, &total_failed);
    
    printf("\n--- Colors ---\n");
    tui::run_colors_tests(&total_passed, &total_failed);
    
    printf("\n--- Signal ---\n");
    tui::run_signal_tests(&total_passed, &total_failed);
    
    printf("\n--- Event ---\n");
    tui::run_event_tests(&total_passed, &total_failed);
    
    printf("\n--- Widgets ---\n");
    tui::run_widget_tests(&total_passed, &total_failed);
    
    printf("\n=== Final Results: %d passed, %d failed ===\n", total_passed, total_failed);
    return total_failed > 0 ? 1 : 0;
}
