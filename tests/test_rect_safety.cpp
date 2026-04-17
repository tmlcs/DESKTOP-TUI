#include "core/rect.hpp"
#include <cassert>
#include <iostream>
#include <limits>

namespace tui {

void test_intersection_normal() {
    Rect a{0, 0, 10, 10};
    Rect b{5, 5, 10, 10};
    
    auto result = a.intersection(b);
    assert(result.has_value());
    assert(result->x == 5);
    assert(result->y == 5);
    assert(result->w == 5);
    assert(result->h == 5);
    std::cout << "✓ Normal intersection test passed" << std::endl;
}

void test_intersection_no_overlap() {
    Rect a{0, 0, 5, 5};
    Rect b{10, 10, 5, 5};
    
    auto result = a.intersection(b);
    assert(!result.has_value());
    std::cout << "✓ No overlap intersection test passed" << std::endl;
}

void test_intersection_edge_touching() {
    Rect a{0, 0, 5, 5};
    Rect b{5, 0, 5, 5};  // Touches at edge but no overlap
    
    auto result = a.intersection(b);
    assert(!result.has_value());  // Should return nullopt for zero-width intersection
    std::cout << "✓ Edge touching intersection test passed" << std::endl;
}

void test_intersection_large_values() {
    // Test with large values close to INT_MAX
    int large = std::numeric_limits<int>::max() / 2;
    Rect a{large, large, 100, 100};
    Rect b{large + 50, large + 50, 100, 100};
    
    auto result = a.intersection(b);
    assert(result.has_value());
    assert(result->w == 50);
    assert(result->h == 50);
    std::cout << "✓ Large values intersection test passed" << std::endl;
}

void test_intersection_negative_coords() {
    Rect a{-10, -10, 20, 20};  // [-10, 10] x [-10, 10]
    Rect b{-5, -5, 20, 20};    // [-5, 15] x [-5, 15]
    
    auto result = a.intersection(b);
    assert(result.has_value());
    assert(result->x == -5);   // max(-10, -5)
    assert(result->y == -5);   // max(-10, -5)
    assert(result->w == 15);   // min(10, 15) - (-5) = 15
    assert(result->h == 15);   // min(10, 15) - (-5) = 15
    std::cout << "✓ Negative coordinates intersection test passed" << std::endl;
}

void test_clamp_within_bounds() {
    Rect bounds{0, 0, 100, 100};
    Rect r{50, 50, 200, 200};  // Extends beyond bounds
    
    auto clamped = r.clamp(bounds);
    assert(clamped.x == 50);
    assert(clamped.y == 50);
    assert(clamped.w == 50);  // Clamped to bounds
    assert(clamped.h == 50);
    std::cout << "✓ Clamp within bounds test passed" << std::endl;
}

void test_clamp_outside_bounds() {
    Rect bounds{0, 0, 100, 100};
    Rect r{150, 150, 50, 50};  // Completely outside bounds
    
    auto clamped = r.clamp(bounds);
    assert(clamped.w == 0);  // Should be empty
    assert(clamped.h == 0);
    std::cout << "✓ Clamp outside bounds test passed" << std::endl;
}

} // namespace tui

namespace tui {

void run_rect_safety_tests() {
    std::cout << "=== Rect Safety Tests ===" << std::endl;
    test_intersection_normal();
    test_intersection_no_overlap();
    test_intersection_edge_touching();
    test_intersection_large_values();
    test_intersection_negative_coords();
    test_clamp_within_bounds();
    test_clamp_outside_bounds();
    std::cout << "All rect safety tests passed!" << std::endl;
}

} // namespace tui
