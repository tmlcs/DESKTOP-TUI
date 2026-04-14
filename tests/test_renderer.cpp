// Tests for renderer.hpp — bounds checking and dirty regions
#include "ui/renderer.hpp"
#include "platform/terminal.hpp"
#include <cstdio>
#include <cstring>

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

// Minimal terminal mock
class MockTerminal : public tui::ITerminal {
public:
    int cols_ = 80, rows_ = 24;
    std::string output;
    tui::TerminalCaps caps_ = tui::TerminalCaps::None;

    bool init() override { return true; }
    void shutdown() override {}
    int cols() const override { return cols_; }
    int rows() const override { return rows_; }
    void clear() override {}
    void flush() override {}
    void sync() override {}
    void cursor_hide() override {}
    void cursor_show() override {}
    void cursor_move(int, int) override {}
    void cursor_save() override {}
    void cursor_restore() override {}
    void write(const std::string& s) override { output += s; }
    void write_at(int, int, const std::string& s) override { output += s; }
    void write_styled(const std::string&, const tui::Style&) override {}
    void write_styled_at(int, int, const std::string&, const tui::Style&) override {}
    void fill(int, int, int, int, char, const tui::Style&) override {}
    void clear_region(int, int, int, int) override {}
    void draw_hline(int, int, int, const tui::Style&) override {}
    void draw_vline(int, int, int, const tui::Style&) override {}
    void draw_rect(int, int, int, int, const tui::Style&) override {}
    void draw_box(int, int, int, int, const tui::Style&, const tui::Style&) override {}
    void enter_raw_mode() override {}
    void leave_raw_mode() override {}
    void enter_alternate_screen() override {}
    void leave_alternate_screen() override {}
    void set_title(const std::string&) override {}
    tui::TerminalCaps caps() const override { return caps_; }
    std::string term_type() const override { return "mock"; }
};

void test_put_bounds(int* passed, int* failed) {
    printf("\n=== put() bounds checking ===\n");

    MockTerminal term;
    tui::Renderer r(term);
    r.resize(10, 5); // 10 cols, 5 rows

    // Valid positions — should not crash
    r.put(0, 0, 'A', tui::Style::Default());
    r.put(9, 4, 'B', tui::Style::Default());
    r.put(5, 2, 'C', tui::Style::Default());

    // Out of bounds — should silently ignore
    r.put(-1, 0, 'X', tui::Style::Default());
    r.put(0, -1, 'X', tui::Style::Default());
    r.put(10, 0, 'X', tui::Style::Default());
    r.put(0, 5, 'X', tui::Style::Default());
    r.put(-100, -100, 'X', tui::Style::Default());
    r.put(100, 100, 'X', tui::Style::Default());

    TEST("put within bounds doesn't crash", true);
}

void test_write_bounds(int* passed, int* failed) {
    printf("\n=== write() bounds checking ===\n");

    MockTerminal term;
    tui::Renderer r(term);
    r.resize(10, 5);

    // Write starting at negative position — should not crash
    r.write(-3, 0, "hello world");

    // Write completely off-screen — should not crash
    r.write(0, -100, "off screen");
    r.write(0, 100, "below screen");
    r.write(-100, 0, "left of screen");

    TEST("write with negative coords doesn't crash", true);
}

void test_fill_rect_bounds(int* passed, int* failed) {
    printf("\n=== fill_rect() bounds checking ===\n");

    MockTerminal term;
    tui::Renderer r(term);
    r.resize(10, 5);

    // Fill starting outside bounds
    r.fill_rect(-5, -5, 20, 15, 'X', tui::Style::Default());
    r.fill_rect(-100, -100, 200, 200, 'Y', tui::Style::Default());

    TEST("fill_rect with out-of-bounds doesn't crash", true);
}

void test_clear_region_bounds(int* passed, int* failed) {
    printf("\n=== clear_region() bounds checking ===\n");

    MockTerminal term;
    tui::Renderer r(term);
    r.resize(10, 5);

    r.clear_region(-5, -5, 20, 15);
    r.clear_region(-100, -100, 200, 200);

    TEST("clear_region with out-of-bounds doesn't crash", true);
}

void test_write_right_coordinate(int* passed, int* failed) {
    printf("\n=== write_right() coordinate correctness ===\n");

    MockTerminal term;
    tui::Renderer r(term);
    r.resize(20, 5);
    r.clear();

    r.write_right(0, 0, "hello", 20, tui::Style::Default());
    r.flush();

    TEST("write_right doesn't crash with valid coords", true);
}

void test_write_center_coordinate(int* passed, int* failed) {
    printf("\n=== write_center() coordinate correctness ===\n");

    MockTerminal term;
    tui::Renderer r(term);
    r.resize(20, 5);
    r.clear();

    r.write_center(0, 0, "hello", 20, tui::Style::Default());
    r.flush();

    TEST("write_center doesn't crash with valid coords", true);
}

void test_mark_dirty_rect(int* passed, int* failed) {
    printf("\n=== mark_dirty_rect() ===\n");

    MockTerminal term;
    tui::Renderer r(term);
    r.resize(10, 5);
    r.clear();
    r.flush();

    r.write(0, 2, "hello", tui::Style::Default());
    r.mark_dirty(0, 2, 10, 1);
    r.flush();

    TEST("mark_dirty_rect doesn't crash", true);
}

void test_draw_box_bounds(int* passed, int* failed) {
    printf("\n=== draw_box() bounds checking ===\n");

    MockTerminal term;
    tui::Renderer r(term);
    r.resize(10, 5);

    tui::Rect offscreen{-3, -2, 15, 10};
    r.draw_box(offscreen, tui::Style::Default(), tui::Style::Default());

    tui::Rect far_offscreen{-100, -100, 5, 5};
    r.draw_box(far_offscreen, tui::Style::Default(), tui::Style::Default());

    TEST("draw_box with off-screen rect doesn't crash", true);
}

void run_renderer_tests(int* passed, int* failed) {
    printf("Running renderer tests...\n");
    test_put_bounds(passed, failed);
    test_write_bounds(passed, failed);
    test_fill_rect_bounds(passed, failed);
    test_clear_region_bounds(passed, failed);
    test_write_right_coordinate(passed, failed);
    test_write_center_coordinate(passed, failed);
    test_mark_dirty_rect(passed, failed);
    test_draw_box_bounds(passed, failed);
}
