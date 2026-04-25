#include "desktop/desktop_manager.hpp"
#include <cassert>
#include <iostream>

namespace tui {

void test_switch_to_negative_index() {
    DesktopManager dm;
    dm.add_desktop("Desktop 2");
    dm.add_desktop("Desktop 3");
    
    // P0: Should return false for negative index
    [[maybe_unused]] bool result = dm.switch_to(-1);
    assert(!result);
    assert(dm.active_index() == 0);  // Should remain on first desktop
    std::cout << "✓ Negative index switch test passed" << std::endl;
}

void test_switch_to_out_of_bounds() {
    DesktopManager dm;
    dm.add_desktop("Desktop 2");
    
    // P0: Should return false for out-of-bounds index
    [[maybe_unused]] bool result = dm.switch_to(10);
    assert(!result);
    assert(dm.active_index() == 0);  // Should remain on first desktop
    std::cout << "✓ Out-of-bounds index switch test passed" << std::endl;
}

void test_switch_to_valid_index() {
    DesktopManager dm;
    dm.add_desktop("Desktop 2");
    dm.add_desktop("Desktop 3");
    
    [[maybe_unused]] bool result = dm.switch_to(1);
    assert(result);
    assert(dm.active_index() == 1);
    assert(dm.active_desktop()->name() == "Desktop 2");
    std::cout << "✓ Valid index switch test passed" << std::endl;
}

void test_remove_desktop_before_active() {
    DesktopManager dm;
    auto d2 = dm.add_desktop("Desktop 2");
    auto d3 = dm.add_desktop("Desktop 3");
    
    dm.switch_to(2);  // Switch to Desktop 3 (index 2)
    assert(dm.active_index() == 2);
    
    // Remove Desktop 2 (index 1, which is before active)
    dm.remove_desktop(d2->id());
    
    // Active index should shift down
    assert(dm.active_index() == 1);
    assert(dm.desktop_count() == 2);
    std::cout << "✓ Remove desktop before active test passed" << std::endl;
}

void test_remove_active_desktop() {
    DesktopManager dm;
    auto d2 = dm.add_desktop("Desktop 2");
    
    dm.switch_to(1);  // Switch to Desktop 2 (index 1)
    assert(dm.active_index() == 1);
    
    // Remove the active desktop
    dm.remove_desktop(d2->id());
    
    // Should switch to remaining desktop
    assert(dm.active_index() == 0);
    assert(dm.desktop_count() == 1);
    assert(dm.active_desktop() != nullptr);
    std::cout << "✓ Remove active desktop test passed" << std::endl;
}

void test_remove_last_desktop_prevented() {
    DesktopManager dm;
    auto d1 = dm.active_desktop();
    
    // Try to remove the only desktop - should be prevented
    dm.remove_desktop(d1->id());
    
    // Should still have one desktop
    assert(dm.desktop_count() == 1);
    std::cout << "✓ Remove last desktop prevention test passed" << std::endl;
}

void test_initial_desktop_activated() {
    DesktopManager dm;
    
    // FIX C2: First desktop should be activated on construction
    assert(dm.active_desktop() != nullptr);
    assert(dm.active_index() == 0);
    assert(dm.active_desktop()->is_active());
    std::cout << "✓ Initial desktop activation test passed" << std::endl;
}

} // namespace tui

namespace tui {

void run_desktop_manager_tests() {
    std::cout << "=== Desktop Manager Safety Tests ===" << std::endl;
    test_switch_to_negative_index();
    test_switch_to_out_of_bounds();
    test_switch_to_valid_index();
    test_remove_desktop_before_active();
    test_remove_active_desktop();
    test_remove_last_desktop_prevented();
    test_initial_desktop_activated();
    std::cout << "All desktop manager tests passed!" << std::endl;
}

} // namespace tui
