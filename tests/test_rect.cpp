// Tests for rect.hpp - Point and Rect functionality
#include "core/rect.hpp"
#include <cstdio>
#include <cstring>

namespace tui {

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

void test_point_basic(int* passed, int* failed) {
    printf("\n=== Point basic operations ===\n");

    Point p1;
    TEST("default constructor", p1.x == 0 && p1.y == 0);

    Point p2(5, 10);
    TEST("parameterized constructor", p2.x == 5 && p2.y == 10);

    Point p3 = p2;
    TEST("copy constructor", p3.x == 5 && p3.y == 10);

    p3 = p1;
    TEST("assignment operator", p3.x == 0 && p3.y == 0);

    TEST("equality operator", Point(5, 10) == Point(5, 10));
    TEST("inequality operator", Point(5, 10) != Point(5, 11));
}

void test_point_operations(int* passed, int* failed) {
    printf("\n=== Point arithmetic operations ===\n");

    Point p1(5, 10);
    Point p2(3, 7);

    Point sum = p1 + p2;
    TEST("addition", sum.x == 8 && sum.y == 17);

    Point diff = p1 - p2;
    TEST("subtraction", diff.x == 2 && diff.y == 3);
}

void test_rect_basic(int* passed, int* failed) {
    printf("\n=== Rect basic operations ===\n");

    Rect r1;
    TEST("default constructor", r1.x == 0 && r1.y == 0 && r1.w == 0 && r1.h == 0);

    Rect r2(10, 20, 30, 40);
    TEST("parameterized constructor", r2.x == 10 && r2.y == 20 && r2.w == 30 && r2.h == 40);

    Point pos(10, 20);
    Rect r3(pos, 30, 40);
    TEST("point+w+h constructor", r3.x == 10 && r3.y == 20 && r3.w == 30 && r3.h == 40);

    TEST("equality operator", Rect(10, 20, 30, 40) == Rect(10, 20, 30, 40));
    TEST("inequality via !=", !(Rect(10, 20, 30, 40) == Rect(10, 20, 31, 40)));
}

void test_rect_properties(int* passed, int* failed) {
    printf("\n=== Rect properties ===\n");

    Rect r(10, 20, 30, 40);

    TEST("left()", r.left() == 10);
    TEST("right()", r.right() == 40);  // x + w = 10 + 30 = 40
    TEST("top()", r.top() == 20);
    TEST("bottom()", r.bottom() == 60);  // y + h = 20 + 40 = 60

    Point center = r.center();
    TEST("center() x", center.x == 25); // 10 + 30/2 = 25
    TEST("center() y", center.y == 40); // 20 + 40/2 = 40

    TEST("empty() false", !r.empty());

    Rect empty;
    TEST("empty() true", empty.empty());

    Rect negative(10, 20, -5, -10);
    TEST("empty() negative size", negative.empty());
}

void test_rect_contains(int* passed, int* failed) {
    printf("\n=== Rect contains ===\n");

    Rect r(10, 20, 30, 40);

    TEST("contains top-left", r.contains(10, 20));
    TEST("contains near bottom-right", r.contains(39, 59));
    TEST("contains center", r.contains(25, 40));
    TEST("contains inside", r.contains(15, 25));

    TEST("not contains left", !r.contains(9, 20));
    TEST("not contains right", !r.contains(40, 20));
    TEST("not contains top", !r.contains(10, 19));
    TEST("not contains bottom", !r.contains(10, 60));
    TEST("not contains outside", !r.contains(100, 100));
}

void test_rect_intersection(int* passed, int* failed) {
    printf("\n=== Rect intersection ===\n");

    Rect r1(0, 0, 20, 20);
    Rect r2(10, 10, 20, 20);

    auto intersect = r1.intersection(r2);
    TEST("intersection exists", intersect.has_value());
    if (intersect.has_value()) {
        TEST("intersection rect", intersect->x == 10 && intersect->y == 10 && 
                                     intersect->w == 10 && intersect->h == 10);
    }

    Rect r3(0, 0, 10, 10);
    Rect r4(20, 20, 10, 10);

    auto no_intersect = r3.intersection(r4);
    TEST("no intersection", !no_intersect.has_value());

    Rect r5(0, 0, 10, 10);
    Rect r6(5, 5, 10, 10);

    auto partial = r5.intersection(r6);
    TEST("partial intersection exists", partial.has_value());
    if (partial.has_value()) {
        TEST("partial intersection rect", partial->x == 5 && partial->y == 5 && 
                                        partial->w == 5 && partial->h == 5);
    }
}

void test_rect_union(int* passed, int* failed) {
    printf("\n=== Rect intersection/intersects ===\n");

    Rect r1(0, 0, 20, 20);
    Rect r2(10, 10, 20, 20);
    TEST("intersects true", r1.intersects(r2));

    Rect r3(0, 0, 10, 10);
    Rect r4(20, 20, 10, 10);
    TEST("intersects false", !r3.intersects(r4));
}

void test_rect_inflate_deflate(int* passed, int* failed) {
    printf("\n=== Rect expand/shrink ===\n");

    Rect r(10, 20, 30, 40);

    Rect expanded = r.expand(5);
    TEST("expand", expanded.x == 5 && expanded.y == 15 && 
                      expanded.w == 40 && expanded.h == 50);

    Rect shrunk = r.shrink(5);
    TEST("shrink", shrunk.x == 15 && shrunk.y == 25 && 
                       shrunk.w == 20 && shrunk.h == 30);
}

void test_rect_intersects(int* passed, int* failed) {
    printf("\n=== Rect edge cases ===\n");

    // Edge touching: r1.right() == r2.x, so they don't actually overlap
    Rect r1(0, 0, 10, 10);
    Rect r2(10, 0, 10, 10);
    TEST("edge touching (no intersection)", !r1.intersects(r2));
}

void run_rect_tests(int* passed, int* failed) {
    printf("Running rect tests...\n");
    test_point_basic(passed, failed);
    test_point_operations(passed, failed);
    test_rect_basic(passed, failed);
    test_rect_properties(passed, failed);
    test_rect_contains(passed, failed);
    test_rect_intersection(passed, failed);
    test_rect_union(passed, failed);
    test_rect_inflate_deflate(passed, failed);
    test_rect_intersects(passed, failed);
}

} // namespace tui
