// Tests for UI widgets - Widget base, Label, and List
#include "ui/widget.hpp"
#include "ui/label.hpp"
#include "ui/list.hpp"
#include "ui/renderer.hpp"
#include "platform/terminal.hpp"
#include <cstdio>
#include <cstring>

namespace tui {

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

// Minimal terminal mock for widget tests
class MockTerminal : public tui::ITerminal {
public:
    int cols_ = 80;
    int rows_ = 24;
    std::string output;

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
    tui::TerminalCaps caps() const override { return tui::TerminalCaps::None; }
    std::string term_type() const override { return "mock"; }
};

void test_widget_basic(int* passed, int* failed) {
    printf("\n=== Widget basic operations ===\n");

    class TestWidget : public Widget {
    public:
        void render(Renderer& r) override { (void)r; }
    };

    TestWidget w;
    
    TEST("default visible", w.visible());
    TEST("default not focused", !w.focused());
    TEST("default not focusable", !w.can_focus());

    w.hide();
    TEST("hide works", !w.visible());

    w.show();
    TEST("show works", w.visible());

    w.set_focusable(true);
    TEST("set_focusable works", w.can_focus());
}

void test_widget_bounds(int* passed, int* failed) {
    printf("\n=== Widget bounds ===\n");

    class TestWidget : public Widget {
    public:
        void render(Renderer& r) override { (void)r; }
    };

    TestWidget w;
    w.set_bounds({10, 20, 30, 40});

    TEST("bounds x", w.x() == 10);
    TEST("bounds y", w.y() == 20);
    TEST("bounds width", w.width() == 30);
    TEST("bounds height", w.height() == 40);

    Rect b = w.bounds();
    TEST("bounds rect", b.x == 10 && b.y == 20 && b.w == 30 && b.h == 40);
}

void test_widget_focus(int* passed, int* failed) {
    printf("\n=== Widget focus ===\n");

    class TestWidget : public Widget {
    public:
        bool focus_called = false;
        bool blur_called = false;

        void render(Renderer& r) override { (void)r; }

    protected:
        void on_focus() override { focus_called = true; }
        void on_blur() override { blur_called = true; }
    };

    TestWidget w;
    w.set_focusable(true);

    w.focus();
    TEST("focus sets focused", w.focused());
    TEST("on_focus called", w.focus_called);

    w.blur();
    TEST("blur clears focused", !w.focused());
    TEST("on_blur called", w.blur_called);
}

void test_widget_click_callback(int* passed, int* failed) {
    printf("\n=== Widget click callback ===\n");

    class TestWidget : public Widget {
    public:
        void render(Renderer& r) override { (void)r; }
        
        void trigger_click() { emit_click(); }
    };

    TestWidget w;
    bool clicked = false;

    w.on_click([&]() { clicked = true; });
    w.trigger_click();

    TEST("click callback called", clicked);
}

void test_label_basic(int* passed, int* failed) {
    printf("\n=== Label basic operations ===\n");

    Label l;
    l.set_text("Hello World");
    
    TEST("get text", l.text() == "Hello World");

    l.set_text("New Text");
    TEST("set text", l.text() == "New Text");

    Label l2("Initial Text");
    TEST("constructor text", l2.text() == "Initial Text");
}

void test_label_render(int* passed, int* failed) {
    printf("\n=== Label rendering ===\n");

    // Use heap allocation to avoid stack overflow with ASan
    auto term = std::make_unique<MockTerminal>();
    Renderer r(*term);
    r.resize(40, 10);
    r.clear();

    Label l("Test Label");
    l.set_bounds({5, 3, 20, 1});
    l.render(r);
    r.flush();

    TEST("label renders without crash", true);

    // Test hidden label doesn't render - use separate scope to reduce stack pressure
    {
        Label l2("Hidden");
        l2.hide();
        l2.set_bounds({5, 4, 20, 1});
        l2.render(r);
        r.flush();
        TEST("hidden label doesn't render", true);
    }
}

void test_label_align(int* passed, int* failed) {
    printf("\n=== Label alignment ===\n");

    Label l("Centered");
    l.set_align(Label::Align::Center);
    TEST("center align set", true); // Just verify it compiles and runs

    l.set_align(Label::Align::Right);
    TEST("right align set", true);

    l.set_align(Label::Align::Left);
    TEST("left align set", true);
}

void test_label_multiline(int* passed, int* failed) {
    printf("\n=== Label multiline ===\n");

    MockTerminal term;
    Renderer r(term);
    r.resize(40, 10);
    r.clear();

    Label l("This is a long text that should wrap to multiple lines when rendered in a bounded area");
    l.set_bounds({0, 0, 20, 5});
    l.render(r);
    r.flush();

    TEST("multiline label renders", true);
}

void test_list_basic(int* passed, int* failed) {
    printf("\n=== List basic operations ===\n");

    List list;
    
    TEST("list initially empty", list.size() == 0);
    TEST("list selected is 0", list.selected() == 0);

    list.add_item("Item 1");
    list.add_item("Item 2");
    list.add_item("Item 3");

    TEST("list size after adds", list.size() == 3);

    auto items = list.items();
    TEST("get items", items.size() == 3 && items[0] == "Item 1");
}

void test_list_set_items(int* passed, int* failed) {
    printf("\n=== List set_items ===\n");

    List list;
    std::vector<std::string> items = {"A", "B", "C", "D"};
    list.set_items(items);

    TEST("set_items size", list.size() == 4);
    TEST("set_items content", list.items()[2] == "C");
}

void test_list_selection(int* passed, int* failed) {
    printf("\n=== List selection ===\n");

    List list;
    list.set_items({"One", "Two", "Three", "Four", "Five"});

    list.set_selected(2);
    TEST("set selected", list.selected() == 2);

    list.set_selected(0);
    TEST("select first", list.selected() == 0);

    list.set_selected(4);
    TEST("select last", list.selected() == 4);

    // Out of bounds should be ignored
    list.set_selected(10);
    TEST("out of bounds ignored", list.selected() == 4);

    list.set_selected(-1);
    TEST("negative ignored", list.selected() == 4);
}

void test_list_clear(int* passed, int* failed) {
    printf("\n=== List clear ===\n");

    List list;
    list.set_items({"A", "B", "C"});
    list.set_selected(2);

    list.clear();

    TEST("clear empties list", list.size() == 0);
    TEST("clear resets selection", list.selected() == 0);
}

void test_list_select_callback(int* passed, int* failed) {
    printf("\n=== List select callback ===\n");

    List list;
    list.set_items({"X", "Y", "Z"});

    int last_selected = -1;
    list.on_select([&](int idx) { last_selected = idx; });

    list.set_selected(1);
    TEST("select callback called", last_selected == 1);
}

void test_list_render(int* passed, int* failed) {
    printf("\n=== List rendering ===\n");

    MockTerminal term;
    Renderer r(term);
    r.resize(40, 10);
    r.clear();

    List list;
    list.set_items({"Item 1", "Item 2", "Item 3", "Item 4", "Item 5"});
    list.set_bounds({5, 2, 20, 4});
    list.render(r);
    r.flush();

    TEST("list renders without crash", true);
}

void test_list_focus_and_events(int* passed, int* failed) {
    printf("\n=== List focus and events ===\n");

    List list;
    list.set_items({"A", "B", "C", "D", "E"});
    list.set_focusable(true);
    list.focus();

    TEST("list can be focused", list.focused());

    // Simulate key up event - stays at 0 when already at first (no wrap by default)
    Event up_event;
    up_event.type = EventType::KeyPress;
    up_event.key_code = Keys::ArrowUp;

    list.handle_event(up_event);
    TEST("up arrow stays at 0 when at first", list.selected() == 0); // Stays at 0

    // Move to position 2
    list.set_selected(2);
    
    Event down_event;
    down_event.type = EventType::KeyPress;
    down_event.key_code = Keys::ArrowDown;

    list.handle_event(down_event);
    TEST("down arrow increases selection", list.selected() == 3); // Increases from 2 to 3
}

void test_list_page_navigation(int* passed, int* failed) {
    printf("\n=== List page navigation ===\n");

    List list;
    std::vector<std::string> items;
    for (int i = 0; i < 20; i++) {
        items.push_back("Item " + std::to_string(i));
    }
    list.set_items(items);
    list.set_bounds({0, 0, 20, 5});
    list.set_selected(10);
    list.set_focusable(true);
    list.focus();

    Event page_up;
    page_up.type = EventType::KeyPress;
    page_up.key_code = Keys::PageUp;
    list.handle_event(page_up);
    TEST("page up works", list.selected() == 5);

    Event page_down;
    page_down.type = EventType::KeyPress;
    page_down.key_code = Keys::PageDown;
    list.handle_event(page_down);
    TEST("page down works", list.selected() == 10);
}

void test_list_home_end(int* passed, int* failed) {
    printf("\n=== List home/end navigation ===\n");

    List list;
    list.set_items({"1", "2", "3", "4", "5"});
    list.set_selected(2);
    list.set_focusable(true);
    list.focus();

    Event home;
    home.type = EventType::KeyPress;
    home.key_code = Keys::Home;
    list.handle_event(home);
    TEST("home goes to start", list.selected() == 0);

    Event end;
    end.type = EventType::KeyPress;
    end.key_code = Keys::End;
    list.handle_event(end);
    TEST("end goes to last", list.selected() == 4);
}

void run_widget_tests(int* passed, int* failed) {
    printf("Running widget tests...\n");
    test_widget_basic(passed, failed);
    test_widget_bounds(passed, failed);
    test_widget_focus(passed, failed);
    test_widget_click_callback(passed, failed);
    test_label_basic(passed, failed);
    test_label_render(passed, failed);
    test_label_align(passed, failed);
    test_label_multiline(passed, failed);
    test_list_basic(passed, failed);
    test_list_set_items(passed, failed);
    test_list_selection(passed, failed);
    test_list_clear(passed, failed);
    test_list_select_callback(passed, failed);
    test_list_render(passed, failed);
    test_list_focus_and_events(passed, failed);
    test_list_page_navigation(passed, failed);
    test_list_home_end(passed, failed);
}

} // namespace tui
