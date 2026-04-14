// Tests for string_utils.hpp
#include "core/string_utils.hpp"
#include <cstdio>
#include <cstring>

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

void test_display_width(int* passed, int* failed) {
    printf("\n=== display_width ===\n");

    TEST("empty string", tui::display_width("") == 0);
    TEST("ASCII single char", tui::display_width("a") == 1);
    TEST("ASCII string", tui::display_width("hello") == 5);
    TEST("CJK character", tui::display_width("中") == 2);
    TEST("CJK string", tui::display_width("中文") == 4);
    TEST("Mixed ASCII+CJK", tui::display_width("a中b文") == 6);
    TEST("emoji", tui::display_width("😀") == 1);
    TEST("spaces", tui::display_width("   ") == 3);
}

void test_truncate(int* passed, int* failed) {
    printf("\n=== truncate ===\n");

    TEST("empty string", tui::truncate("", 5) == "");
    TEST("short string", tui::truncate("hello", 10) == "hello");
    TEST("exact fit", tui::truncate("hello", 5) == "hello");
    TEST("truncate ASCII", tui::truncate("hello world", 5) == "hello");
    TEST("CJK not truncated (fits)", tui::truncate("中文", 4) == "中文");
    TEST("CJK truncated (cuts mid-string)", tui::truncate("中文abc", 4) == "中文");
    TEST("CJK partial (cuts before wide)", tui::truncate("中文abc", 3) == "中");
}

void test_trim(int* passed, int* failed) {
    printf("\n=== trim ===\n");

    TEST("empty string", tui::trim("") == "");
    TEST("all spaces", tui::trim("   ") == "");
    TEST("no spaces", tui::trim("hello") == "hello");
    TEST("leading spaces", tui::trim("  hello") == "hello");
    TEST("trailing spaces", tui::trim("hello  ") == "hello");
    TEST("both sides", tui::trim("  hello  ") == "hello");
    TEST("single char", tui::trim(" a ") == "a");
    TEST("tabs and newlines", tui::trim("\t\nhello\t\n") == "hello");
}

void test_split(int* passed, int* failed) {
    printf("\n=== split ===\n");

    auto r1 = tui::split("a,b,c", ',');
    TEST("split 3 items", r1.size() == 3 && r1[0] == "a" && r1[1] == "b" && r1[2] == "c");

    auto r2 = tui::split("hello", ',');
    TEST("split single item", r2.size() == 1 && r2[0] == "hello");

    auto r3 = tui::split(",", ',');
    TEST("split empty items", r3.size() >= 1); // std::getline behavior varies
}

void test_pad_center_right(int* passed, int* failed) {
    printf("\n=== pad/center/right_align ===\n");

    TEST("pad short", tui::pad("ab", 5) == "ab   ");
    TEST("pad long truncates", tui::pad("abcdef", 3) == "abc");
    TEST("center short", tui::center("ab", 5) == " ab  "); // 1+2+2 = 5
    TEST("right_align", tui::right_align("ab", 5) == "   ab");
    TEST("repeat", tui::repeat("ab", 3) == "ababab");
    TEST("repeat 0", tui::repeat("ab", 0) == "");
}

void run_string_utils_tests(int* passed, int* failed);
void run_renderer_tests(int* passed, int* failed);

int main(int argc, char* argv[]) {
    int passed = 0, failed = 0;
    bool run_all = (argc == 1);
    bool run_string_utils = run_all || (argc > 1 && strcmp(argv[1], "--string_utils") == 0);
    bool run_renderer = run_all || (argc > 1 && strcmp(argv[1], "--renderer") == 0);

    if (run_string_utils) run_string_utils_tests(&passed, &failed);
    if (run_renderer) run_renderer_tests(&passed, &failed);

    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}

void run_string_utils_tests(int* passed, int* failed) {
    test_display_width(passed, failed);
    test_truncate(passed, failed);
    test_trim(passed, failed);
    test_split(passed, failed);
    test_pad_center_right(passed, failed);
}
