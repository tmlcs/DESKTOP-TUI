// Tests for colors.hpp - Color and Style functionality
#include "core/colors.hpp"
#include <cstdio>
#include <cstring>

namespace tui {

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

void test_color_rgb(int* passed, int* failed) {
    printf("\n=== ColorRGB basic operations ===\n");

    ColorRGB c1;
    TEST("default constructor", c1.r == 0 && c1.g == 0 && c1.b == 0);

    ColorRGB c2(255, 128, 64);
    TEST("parameterized constructor", c2.r == 255 && c2.g == 128 && c2.b == 64);

    TEST("equality same", ColorRGB(255, 128, 64) == ColorRGB(255, 128, 64));
    TEST("equality different", !(ColorRGB(255, 128, 64) == ColorRGB(255, 128, 63)));

    // Common colors
    TEST("Black", ColorRGB::Black() == ColorRGB(0, 0, 0));
    TEST("White", ColorRGB::White() == ColorRGB(255, 255, 255));
    TEST("Red", ColorRGB::Red() == ColorRGB(255, 0, 0));
    TEST("Green", ColorRGB::Green() == ColorRGB(0, 255, 0));
    TEST("Blue", ColorRGB::Blue() == ColorRGB(0, 0, 255));
    TEST("Yellow", ColorRGB::Yellow() == ColorRGB(255, 255, 0));
    TEST("Cyan", ColorRGB::Cyan() == ColorRGB(0, 255, 255));
    TEST("Magenta", ColorRGB::Magenta() == ColorRGB(255, 0, 255));
    TEST("Gray", ColorRGB::Gray() == ColorRGB(128, 128, 128));
    TEST("DarkGray", ColorRGB::DarkGray() == ColorRGB(64, 64, 64));
}

void test_color_modes(int* passed, int* failed) {
    printf("\n=== Color modes ===\n");

    Color c1 = Color::DefaultColor();
    TEST("default color mode", c1.mode == Color::Mode::Default);

    Color c2 = Color::Pal(5);
    TEST("indexed color mode", c2.mode == Color::Mode::Indexed);
    TEST("indexed color value", c2.index == 5);

    Color c3 = Color::RGB(100, 150, 200);
    TEST("truecolor mode", c3.mode == Color::Mode::TrueColor);
    TEST("truecolor r", c3.rgb.r == 100);
    TEST("truecolor g", c3.rgb.g == 150);
    TEST("truecolor b", c3.rgb.b == 200);

    ColorRGB rgb(50, 75, 100);
    Color c4 = Color::RGB(rgb);
    TEST("RGB from struct", c4.rgb.r == 50 && c4.rgb.g == 75 && c4.rgb.b == 100);
}

void test_style_basic(int* passed, int* failed) {
    printf("\n=== Style basic operations ===\n");

    Style s1;
    TEST("default style fg mode", s1.fg.mode == Color::Mode::Default);
    TEST("default style bg mode", s1.bg.mode == Color::Mode::Default);
    TEST("default style not bold", !s1.bold);
    TEST("default style not italic", !s1.italic);
    TEST("default style not underline", !s1.underline);
    TEST("default style not blink", !s1.blink);
    TEST("default style not reverse", !s1.reverse);
    TEST("default style not dim", !s1.dim);
    TEST("default style not hidden", !s1.hidden);

    Style s2 = Style::Default();
    TEST("Default() static", s2.fg.mode == Color::Mode::Default);
}

void test_style_attributes(int* passed, int* failed) {
    printf("\n=== Style attributes ===\n");

    Style s;
    s.bold = true;
    TEST("bold set", s.bold);

    s.italic = true;
    TEST("italic set", s.italic);

    s.underline = true;
    TEST("underline set", s.underline);

    s.blink = true;
    TEST("blink set", s.blink);

    s.reverse = true;
    TEST("reverse set", s.reverse);

    s.dim = true;
    TEST("dim set", s.dim);

    s.hidden = true;
    TEST("hidden set", s.hidden);
}

void test_style_with_methods(int* passed, int* failed) {
    printf("\n=== Style with_* methods ===\n");

    Style base;
    
    Color red = Color::Pal(9);
    Style s1 = base.with_fg(red);
    TEST("with_fg preserves base", !s1.bold && !s1.italic);
    TEST("with_fg sets color", s1.fg.mode == Color::Mode::Indexed && s1.fg.index == 9);

    Color blue = Color::Pal(12);
    Style s2 = base.with_bg(blue);
    TEST("with_bg preserves base", !s2.bold && !s2.italic);
    TEST("with_bg sets color", s2.bg.mode == Color::Mode::Indexed && s2.bg.index == 12);

    Style s3 = base.with_bold(true);
    TEST("with_bold", s3.bold && !s3.italic);

    Style s4 = base.with_reverse(true);
    TEST("with_reverse", s4.reverse && !s4.bold);

    // Chaining
    Style s5 = base.with_fg(red).with_bold(true).with_reverse(true);
    TEST("chaining fg", s5.fg.mode == Color::Mode::Indexed && s5.fg.index == 9);
    TEST("chaining bold", s5.bold);
    TEST("chaining reverse", s5.reverse);
}

void test_style_equality(int* passed, int* failed) {
    printf("\n=== Style equality ===\n");

    Style s1;
    Style s2;
    TEST("default styles equal", s1 == s2);

    Style s3 = s1;
    s3.bold = true;
    TEST("different bold not equal", s1 != s3);

    Style s4 = Style::Default();
    s4.fg = Color::Pal(5);
    Style s5 = Style::Default();
    s5.fg = Color::Pal(5);
    TEST("same indexed fg equal", s4 == s5);

    Style s6 = Style::Default();
    s6.fg = Color::RGB(100, 150, 200);
    Style s7 = Style::Default();
    s7.fg = Color::RGB(100, 150, 200);
    TEST("same truecolor fg equal", s6 == s7);

    Style s8 = Style::Default();
    s8.fg = Color::RGB(100, 150, 200);
    Style s9 = Style::Default();
    s9.fg = Color::RGB(100, 150, 201);
    TEST("different truecolor not equal", s8 != s9);
}

void test_predefined_styles(int* passed, int* failed) {
    printf("\n=== Predefined styles ===\n");

    Style normal = Styles::Normal();
    TEST("Normal is default", normal == Style::Default());

    Style bold = Styles::Bold();
    TEST("Bold has bold", bold.bold);

    Style title = Styles::Title();
    TEST("Title has bold", title.bold);
    TEST("Title has fg", title.fg.mode == Color::Mode::Indexed);

    Style header = Styles::Header();
    TEST("Header has bold", header.bold);
    TEST("Header has fg", header.fg.mode == Color::Mode::Indexed);

    Style success = Styles::Success();
    TEST("Success has fg", success.fg.mode == Color::Mode::Indexed);

    Style error = Styles::Error();
    TEST("Error has fg", error.fg.mode == Color::Mode::Indexed);

    Style warning = Styles::Warning();
    TEST("Warning has fg", warning.fg.mode == Color::Mode::Indexed);

    Style info = Styles::Info();
    TEST("Info has fg", info.fg.mode == Color::Mode::Indexed);

    Style dim = Styles::Dim();
    TEST("Dim has dim", dim.dim);

    Style selected = Styles::Selected();
    TEST("Selected has reverse", selected.reverse);

    Style border = Styles::Border();
    TEST("Border has fg", border.fg.mode == Color::Mode::Indexed);

    Style border_active = Styles::BorderActive();
    TEST("BorderActive has bold", border_active.bold);
    TEST("BorderActive has fg", border_active.fg.mode == Color::Mode::Indexed);
}

void test_emit_style_to_string(int* passed, int* failed) {
    printf("\n=== emit_style_to_string ===\n");

    Style s1 = Style::Default();
    std::string seq1 = emit_style_to_string(s1);
    TEST("default style sequence", seq1 == "\033[0m");

    Style s2 = Style::Default();
    s2.bold = true;
    std::string seq2 = emit_style_to_string(s2);
    TEST("bold sequence contains ;1", seq2.find(";1") != std::string::npos);
    TEST("bold sequence ends with m", seq2.back() == 'm');
    TEST("bold sequence starts with esc", seq2[0] == '\033');

    Style s3 = Style::Default();
    s3.italic = true;
    s3.underline = true;
    std::string seq3 = emit_style_to_string(s3);
    TEST("multiple attrs sequence", seq3.find(";3") != std::string::npos && 
                                     seq3.find(";4") != std::string::npos);

    Style s4 = Style::Default();
    s4.fg = Color::Pal(9);
    std::string seq4 = emit_style_to_string(s4);
    TEST("indexed fg sequence", seq4.find(";38;5;9") != std::string::npos);

    Style s5 = Style::Default();
    s5.fg = Color::RGB(255, 0, 0);
    std::string seq5 = emit_style_to_string(s5);
    TEST("truecolor fg sequence", seq5.find(";38;2;255;0;0") != std::string::npos);

    Style s6 = Style::Default();
    s6.bg = Color::Pal(12);
    std::string seq6 = emit_style_to_string(s6);
    TEST("indexed bg sequence", seq6.find(";48;5;12") != std::string::npos);

    Style s7 = Style::Default();
    s7.bg = Color::RGB(0, 255, 0);
    std::string seq7 = emit_style_to_string(s7);
    TEST("truecolor bg sequence", seq7.find(";48;2;0;255;0") != std::string::npos);

    Style s8;
    s8.bold = true;
    s8.fg = Color::RGB(255, 0, 0);
    s8.bg = Color::Pal(12);
    std::string seq8 = emit_style_to_string(s8);
    TEST("complex sequence has all parts", seq8.find(";1") != std::string::npos &&
                                            seq8.find(";38;2;255;0;0") != std::string::npos &&
                                            seq8.find(";48;5;12") != std::string::npos);
}

void run_colors_tests(int* passed, int* failed) {
    printf("Running colors tests...\n");
    test_color_rgb(passed, failed);
    test_color_modes(passed, failed);
    test_style_basic(passed, failed);
    test_style_attributes(passed, failed);
    test_style_with_methods(passed, failed);
    test_style_equality(passed, failed);
    test_predefined_styles(passed, failed);
    test_emit_style_to_string(passed, failed);
}

} // namespace tui
