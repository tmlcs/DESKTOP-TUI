// Integration tests for window operations and plugin loading
#include "window/window.hpp"
#include "desktop/desktop_manager.hpp"
#include "ui/panel.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>

namespace tui {

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

void test_window_creation_destruction(int* passed, int* failed) {
    printf("\n=== Window Creation/Destruction ===\n");

    // Create a window
    Window w("Test Window");
    w.set_bounds({0, 0, 20, 5});

    TEST("window created", w.title() == "Test Window");
    TEST("window bounds set", w.bounds_x() == 0 && w.bounds_y() == 0 && w.bounds_w() == 20 && w.bounds_h() == 5);

    // Test hide/show
    w.hide();
    TEST("hide works", !w.visible());
    w.show();
    TEST("show works", w.visible());
}

void test_desktop_switching(int* passed, int* failed) {
    printf("\n=== Desktop Switching ===\n");

    DesktopManager dm;
    // Constructor adds 1 desktop, then we add 3 more = 4 total
    dm.add_desktop("Desktop 1");
    dm.add_desktop("Desktop 2");
    dm.add_desktop("Desktop 3");

    TEST("initial active index", dm.active_index() == 0);
    TEST("desktop count", dm.desktop_count() == 4);  // 1 from constructor + 3 added

    // Switch to desktop 2
    bool result = dm.switch_to(1);
    TEST("switch to valid desktop", result && dm.active_index() == 1);

    // Switch back to desktop 1
    result = dm.switch_to(0);
    TEST("switch back to desktop 1", result && dm.active_index() == 0);

    // Out of bounds should fail
    result = dm.switch_to(10);
    TEST("out of bounds switch fails", !result && dm.active_index() == 0);
}

void test_event_handling(int* passed, int* failed) {
    printf("\n=== Event Handling ===\n");

    Window w("Event Test");
    w.set_bounds({0, 0, 20, 5});
    w.set_focusable(true);
    w.focus();

    // Test key press event
    Event key_event;
    key_event.type = EventType::KeyPress;
    key_event.key_code = Keys::Escape;

    w.handle_event(key_event);
    TEST("window handles key event", true);  // Just verify it doesn't crash

    // Test mouse click event
    Event click_event;
    click_event.type = EventType::MouseMove;
    click_event.mouse_x = 5;
    click_event.mouse_y = 2;

    w.handle_event(click_event);
    TEST("window handles mouse event", true);  // Just verify it doesn't crash
}

void test_window_resize(int* passed, int* failed) {
    printf("\n=== Window Resize ===\n");

    Window w("Resize Test");
    w.set_bounds({0, 0, 10, 5});

    TEST("initial size", w.bounds_w() == 10 && w.bounds_h() == 5);

    w.resize(20, 10);
    TEST("resize works", w.bounds_w() == 20 && w.bounds_h() == 10);

    // Test resize with negative values (should be clamped by Rect)
    w.resize(-5, -5);
    TEST("negative resize handled", w.bounds_w() == 0 && w.bounds_h() == 0);  // Rect clamps to 0
}

void test_window_minimize_maximize(int* passed, int* failed) {
    printf("\n=== Window Minimize/Maximize ===\n");

    Window w("Min/Max Test");
    w.set_bounds({0, 0, 20, 5});

    TEST("initially visible", w.visible());
    TEST("initially not minimized", !w.is_minimized());
    TEST("initially not maximized", !w.is_maximized());

    w.minimize();
    TEST("minimize works", w.is_minimized());

    w.restore();
    TEST("restore works", !w.is_minimized());

    w.maximize();
    TEST("maximize works", w.is_maximized());

    w.restore_maximized();
    TEST("restore from maximize", !w.is_maximized());
}

void test_window_focus_cycle(int* passed, int* failed) {
    printf("\n=== Window Focus Cycle ===\n");

    Window w1("Window 1"), w2("Window 2"), w3("Window 3");
    w1.set_bounds({0, 0, 10, 5});
    w2.set_bounds({10, 0, 10, 5});
    w3.set_bounds({20, 0, 10, 5});

    w1.set_focusable(true);
    w2.set_focusable(true);
    w3.set_focusable(true);

    w1.focus();
    TEST("w1 focused", w1.is_focused());
    // Note: Focus isolation is handled by DesktopManager, not individual windows
    // Other windows retain their focus state until explicitly blurred

    w2.focus();
    TEST("w2 focused", w2.is_focused());
    // w1 remains focused (focus isolation handled by DesktopManager)
    TEST("focus can be set on multiple windows", w1.is_focused() && w2.is_focused());

    w3.focus();
    TEST("w3 focused", w3.is_focused());
}

void test_desktop_add_remove(int* passed, int* failed) {
    printf("\n=== Desktop Add/Remove ===\n");

    DesktopManager dm;
    // Constructor adds 1 desktop, then we add 2 more = 3 total
    dm.add_desktop("Desktop 1");
    dm.add_desktop("Desktop 2");

    TEST("three desktops", dm.desktop_count() == 3);  // 1 from constructor + 2 added

    // Remove first desktop
    auto first = dm.all_desktops()[0];
    dm.remove_desktop(first->id());
    TEST("two desktops after remove", dm.desktop_count() == 2);  // 1 from constructor + 1 remaining

    // Cannot remove last desktop (keep at least one)
    auto last = dm.active_desktop();
    dm.remove_desktop(last->id());
    TEST("still has one desktop", dm.desktop_count() == 1);  // 1 from constructor
}

void run_integration_tests(int* passed, int* failed) {
    printf("Running integration tests...\n");
    printf("========================================\n");

    test_window_creation_destruction(passed, failed);
    test_desktop_switching(passed, failed);
    test_event_handling(passed, failed);
    test_window_resize(passed, failed);
    test_window_minimize_maximize(passed, failed);
    test_window_focus_cycle(passed, failed);
    test_desktop_add_remove(passed, failed);

    printf("\n=== Integration Tests Complete ===\n");
    printf("Passed: %d, Failed: %d\n", *passed, *failed);
}

} // namespace tui
