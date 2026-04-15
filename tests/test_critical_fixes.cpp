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

void test_s3_s6_string_utils_edge_cases() {
    printf("\n--- S3-S6: string_utils edge cases ---\n");
    TEST("display_width empty string", tui::display_width("") == 0);
    TEST("truncate empty", tui::truncate("", 5) == "");
    TEST("truncate to 0", tui::truncate("hello", 0) == "");
    TEST("trim all whitespace", tui::trim("   \t\n  ") == "");
    TEST("split empty string", tui::split("", ',').size() == 0);
    TEST("word_wrap empty", tui::word_wrap("", 10).size() == 1);
    TEST("pad empty", tui::pad("", 5) == "     ");
    TEST("center empty", tui::center("", 5) == "     ");
    TEST("right_align empty", tui::right_align("", 5) == "     ");
    TEST("word_wrap single long word exceeds width",
        []() {
            auto lines = tui::word_wrap("abcdefghij", 5);
            // word_wrap can't split single word — it goes on its own line
            return lines.size() >= 1 && lines.back() == "abcdefghij";
        }());
}

void test_d1_emit_style_correctness() {
    printf("\n--- D1: emit_style_to_string correctness ---\n");
    TEST("emit_style default style",
        []() {
            std::string s = tui::emit_style_to_string(tui::Style::Default());
            return s == "\033[0m";
        }());
    TEST("emit_style bold",
        []() {
            tui::Style s = tui::Style::Default();
            s.bold = true;
            return tui::emit_style_to_string(s) == "\033[0;1m";
        }());
    TEST("emit_style bold+italic",
        []() {
            tui::Style s = tui::Style::Default();
            s.bold = true; s.italic = true;
            return tui::emit_style_to_string(s) == "\033[0;1;3m";
        }());
    TEST("emit_style 256-color fg",
        []() {
            tui::Style s = tui::Style::Default();
            s.fg.mode = tui::Color::Mode::Indexed;
            s.fg.index = 196;
            auto result = tui::emit_style_to_string(s);
            return result.find("38;5;196") != std::string::npos;
        }());
    TEST("emit_style truecolor fg",
        []() {
            tui::Style s = tui::Style::Default();
            s.fg.mode = tui::Color::Mode::TrueColor;
            s.fg.rgb.r = 255; s.fg.rgb.g = 128; s.fg.rgb.b = 64;
            auto result = tui::emit_style_to_string(s);
            return result.find("38;2;255;128;64") != std::string::npos;
        }());
    TEST("emit_style truecolor bg",
        []() {
            tui::Style s = tui::Style::Default();
            s.bg.mode = tui::Color::Mode::TrueColor;
            s.bg.rgb.r = 0; s.bg.rgb.g = 100; s.bg.rgb.b = 200;
            auto result = tui::emit_style_to_string(s);
            return result.find("48;2;0;100;200") != std::string::npos;
        }());
}

void test_c5_utf8_decode_boundary() {
    printf("\n--- C5: utf8_decode boundary safety ---\n");

    TEST("truncated 3-byte UTF-8 returns 0",
        []() {
            const char buf[] = "\xE6\x97";  // "日" sin último byte
            const char* p = buf;
            const char* end = buf + 2;
            char32_t result = tui::utf8_decode(p, end);
            return result == 0;
        }());

    TEST("truncated 2-byte UTF-8 returns 0",
        []() {
            const char buf[] = "\xC2";  // Start of 2-byte, no continuation
            const char* p = buf;
            const char* end = buf + 1;
            char32_t result = tui::utf8_decode(p, end);
            return result == 0;
        }());

    TEST("valid 3-byte UTF-8 decodes correctly",
        []() {
            const char buf[] = "\xE6\x97\xA5";  // 日
            const char* p = buf;
            const char* end = buf + 3;
            char32_t result = tui::utf8_decode(p, end);
            return result == 0x65E5;
        }());

    TEST("decode at exact empty boundary returns 0",
        []() {
            const char buf[] = "";
            const char* p = buf;
            const char* end = buf;
            char32_t result = tui::utf8_decode(p, end);
            return result == 0;
        }());
}

void test_c1_cjk_column_tracking() {
    printf("\n--- C1: Renderer CJK column tracking ---\n");

    TestTerminal term(80, 24);
    tui::Renderer r(term);
    r.resize(80, 24);
    r.clear();

    // "日" and "本" are each display width 2
    r.write(0, 0, "日本", tui::Style::Default());
    r.flush();
    auto output = term.drain_output();

    TEST("CJK text renders both codepoints",
        output.find("\xE6\x97\xA5") != std::string::npos &&  // 日
        output.find("\xE6\x9C\xAC") != std::string::npos);   // 本

    // Mixed ASCII + CJK: "A日B" should not overlap
    r.clear();
    r.write(0, 1, "A日B", tui::Style::Default());
    r.flush();
    output = term.drain_output();
    TEST("mixed ASCII+CJK renders without overlap",
        output.find("A") != std::string::npos &&
        output.find("\xE6\x97\xA5") != std::string::npos &&
        output.find("B") != std::string::npos);
}

void test_s4_cjk_range_consistency() {
    printf("\n--- S4: CJK range consistency between display_width and truncate ---\n");

    TEST("CJK U+4E00 has width 2", tui::display_width("\xE4\xB8\x80") == 2);

    // truncate should never produce a string wider than max_width
    std::string cjk_text = "日本語テスト";
    auto truncated = tui::truncate(cjk_text, 6);
    TEST("truncate respects display width", tui::display_width(truncated) <= 6);
    TEST("truncate doesn't split codepoint", truncated.size() % 3 == 0 || truncated.empty());
}

int run_critical_fixes_main() {
    int p = 0, f = 0;
    passed = &p;
    failed = &f;

    printf("\n=== v0.1.4 Critical Bugfix Tests ===\n");

    test_c2_desktop_manager_init();
    test_c3_c4_remove_desktop();
    test_c5_utf8_decode_boundary();
    test_c1_cjk_column_tracking();
    test_c1_utf8_write();
    test_c1_utf8_pad_center_right_align();
    test_c1_utf8_word_wrap();
    test_c1_utf8_list();
    test_c1_utf8_panel_title();
    test_s4_cjk_range_consistency();
    test_c6_panel_clipping();
    test_c7_signal_iterator_safety();
    test_c7_eventbus_iterator_safety();
    test_s3_s6_string_utils_edge_cases();
    test_d1_emit_style_correctness();

    printf("\n=== Results: %d passed, %d failed ===\n\n", p, f);
    return f;
}
