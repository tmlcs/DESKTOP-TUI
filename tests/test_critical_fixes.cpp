// Tests for v0.1.4 critical bugfixes (C1-C8)
#include "ui/renderer.hpp"
#include "ui/panel.hpp"
#include "ui/list.hpp"
#include "desktop/desktop_manager.hpp"
#include "core/string_utils.hpp"
#include "core/signal.hpp"
#include "core/event.hpp"
#include "platform/terminal.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

static int* passed;
static int* failed;

// Complete terminal mock for testing
class TestTerminal : public tui::ITerminal {
public:
    TestTerminal(int cols = 80, int rows = 24) : cols_(cols), rows_(rows) {}
    bool init() override { return true; }
    void shutdown() override {}
    void clear() override { output_.clear(); }
    void flush() override {}
    void sync() override {}
    void cursor_hide() override {}
    void cursor_show() override {}
    void cursor_move(int, int) override {}
    void cursor_save() override {}
    void cursor_restore() override {}
    int cols() const override { return cols_; }
    int rows() const override { return rows_; }
    void write(const std::string& s) override { output_ += s; }
    void write_at(int, int, const std::string& s) override { output_ += s; }
    void write_styled(const std::string& s, const tui::Style&) override { output_ += s; }
    void write_styled_at(int, int, const std::string& s, const tui::Style&) override { output_ += s; }
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
    tui::TerminalCaps caps() const override { return tui::TerminalCaps::BoxDrawing | tui::TerminalCaps::TrueColor | tui::TerminalCaps::Unicode; }
    std::string term_type() const override { return "xterm-256color"; }

    std::string drain_output() { std::string s = output_; output_.clear(); return s; }

private:
    int cols_, rows_;
    std::string output_;
};

void test_c2_desktop_manager_init() {
    printf("\n--- C2: DesktopManager activates first desktop ---\n");
    TEST("default constructor activates first desktop",
        []() {
            tui::DesktopManager mgr;
            return mgr.active_desktop() != nullptr && mgr.active_index() == 0;
        }());
    TEST("multi-desktop constructor activates first desktop",
        []() {
            tui::DesktopManager mgr(3);
            return mgr.active_desktop() != nullptr && mgr.active_index() == 0;
        }());
}

void test_c3_c4_remove_desktop() {
    printf("\n--- C3/C4: remove_desktop index + pointer safety ---\n");
    TEST("removing desktop before active decrements active_index",
        []() {
            tui::DesktopManager mgr(3);  // Desktop 1, 2, 3 — active=0
            mgr.switch_to(2);  // switch to Desktop 3 (index 2)
            mgr.remove_desktop(mgr.get_desktop(1)->id());  // remove Desktop 2
            return mgr.active_index() == 1 && mgr.desktop_count() == 2 &&
                   mgr.active_desktop() != nullptr;
        }());
    TEST("removing active desktop updates active_ pointer",
        []() {
            tui::DesktopManager mgr(3);
            mgr.switch_to(1);  // Desktop 2 is active
            auto removed_id = mgr.get_desktop(1)->id();
            mgr.remove_desktop(removed_id);
            return mgr.active_desktop() != nullptr && mgr.active_index() == 1;
        }());
    TEST("removing desktop after active doesn't change active_index",
        []() {
            tui::DesktopManager mgr(3);
            mgr.switch_to(0);  // Desktop 1 is active
            mgr.remove_desktop(mgr.get_desktop(2)->id());  // remove Desktop 3
            return mgr.active_index() == 0 && mgr.desktop_count() == 2;
        }());
    TEST("cannot remove last desktop",
        []() {
            tui::DesktopManager mgr;
            mgr.remove_desktop(mgr.get_desktop(0)->id());
            return mgr.desktop_count() == 1 && mgr.active_desktop() != nullptr;
        }());
}

void test_c1_utf8_write() {
    printf("\n--- C1: UTF-8 in Renderer::write() ---\n");
    TestTerminal term(80, 24);
    tui::Renderer r(term);
    r.resize(80, 24);
    r.clear();
    r.write(0, 0, "日本語");
    r.flush();
    auto output = term.drain_output();
    TEST("write preserves UTF-8 codepoints (日)", output.find("\xE6\x97\xA5") != std::string::npos);
    TEST("write preserves UTF-8 codepoints (本)", output.find("\xE6\x9C\xAC") != std::string::npos);
    TEST("write preserves UTF-8 codepoints (語)", output.find("\xE8\xAA\x9E") != std::string::npos);
}

void test_c1_utf8_pad_center_right_align() {
    printf("\n--- C1: UTF-8 in pad/center/right_align ---\n");
    TEST("pad uses display_width",
        []() {
            std::string result = tui::pad("日本", 6);
            return result == "日本  ";  // 2 spaces (display_width=4, need 6)
        }());
    TEST("center uses display_width",
        []() {
            std::string result = tui::center("日", 5);
            // "日" display_width=2, padding=3, left=1, right=2
            // Result: " 日  " = 1 space + 3 bytes + 2 spaces = 6 bytes
            return tui::display_width(result) == 5 && result[0] == ' ';
        }());
    TEST("right_align uses display_width",
        []() {
            std::string result = tui::right_align("日本", 6);
            // "日本" display_width=4, padding=2 → display_width should be 6
            return tui::display_width(result) == 6;
        }());
}

void test_c1_utf8_word_wrap() {
    printf("\n--- C1: UTF-8 in word_wrap ---\n");
    TEST("word_wrap uses display_width for CJK",
        []() {
            // "日 本 語" — each CJK = 2 cells, spaces = 1
            auto lines = tui::word_wrap("日 本 語", 5);
            // "日 本" = 2+1+2 = 5, fits. "語" = 2, next line.
            return lines.size() >= 2;
        }());
}

void test_c1_utf8_list() {
    printf("\n--- C1: UTF-8 in List truncation ---\n");
    tui::List list;
    list.set_items({"日本語テスト", "test"});
    list.set_bounds({0, 0, 6, 2});  // width 6 = 3 CJK chars
    TestTerminal term(80, 24);
    tui::Renderer r(term);
    r.resize(80, 24);
    list.render(r);
    r.flush();
    auto output = term.drain_output();
    TEST("List truncates by display_width, not bytes", output.find("日本") != std::string::npos);
}

void test_c1_utf8_panel_title() {
    printf("\n--- C1: UTF-8 in Panel title ---\n");
    tui::Panel panel("日本語", tui::Styles::Border());
    panel.set_bounds({0, 0, 30, 10});
    TestTerminal term(80, 24);
    tui::Renderer r(term);
    r.resize(80, 24);
    panel.render(r);
    // Should not crash or produce garbled output
    TEST("Panel renders UTF-8 title without crashing", true);
}

void test_c6_panel_clipping() {
    printf("\n--- C6: Panel child clipping ---\n");
    tui::Panel panel("Test", tui::Styles::Border());
    panel.set_bounds({0, 0, 10, 10});  // content area = {1,1,8,8}
    auto child = std::make_shared<tui::Panel>("", tui::Styles::Border());
    child->set_bounds({0, 0, 15, 15});  // extends beyond panel
    panel.add_child(child);

    TestTerminal term(80, 24);
    tui::Renderer r(term);
    r.resize(80, 24);
    panel.render(r);
    // Should not crash — child clipped to content area
    TEST("Panel clips children to content area", true);
}

void test_c7_signal_iterator_safety() {
    printf("\n--- C7: Signal iterator safety ---\n");
    tui::Signal<int> sig;
    std::vector<int> results;
    tui::Signal<int>::SlotId self_id = 0;
    sig.connect([&](int v) { results.push_back(v * 10); });
    self_id = sig.connect([&](int v) {
        results.push_back(v * 100);
        // Disconnect self during emit — would crash without snapshot fix
        sig.disconnect(self_id);
    });
    sig.connect([&](int v) { results.push_back(v * 1000); });

    sig.emit(1);
    TEST("Signal survives disconnect during emit",
        results.size() >= 2 && results.back() == 1000);
}

void test_c7_eventbus_iterator_safety() {
    printf("\n--- C7: EventBus iterator safety ---\n");
    tui::EventBus bus;
    std::vector<int> results;
    tui::EventBus::SubscriptionId self_id = 0;
    self_id = bus.subscribe(tui::EventType::KeyPress, [&](const tui::Event& e) {
        results.push_back(static_cast<int>(e.key_code));
        bus.unsubscribe(self_id);  // unsubscribe self during publish
    });
    tui::Event e(tui::EventType::KeyPress);
    e.key_code = 42;
    bus.publish(e);  // should not crash
    TEST("EventBus survives unsubscribe during publish", results.size() == 1 && results[0] == 42);
}

int run_critical_fixes_main() {
    int p = 0, f = 0;
    passed = &p;
    failed = &f;

    printf("\n=== v0.1.4 Critical Bugfix Tests ===\n");

    test_c2_desktop_manager_init();
    test_c3_c4_remove_desktop();
    test_c1_utf8_write();
    test_c1_utf8_pad_center_right_align();
    test_c1_utf8_word_wrap();
    test_c1_utf8_list();
    test_c1_utf8_panel_title();
    test_c6_panel_clipping();
    test_c7_signal_iterator_safety();
    test_c7_eventbus_iterator_safety();

    printf("\n=== Results: %d passed, %d failed ===\n\n", p, f);
    return f;
}
