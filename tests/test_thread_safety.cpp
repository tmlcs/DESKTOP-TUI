#include "../src/ui/text_input.hpp"
#include <thread>
#include <cassert>
#include <iostream>

namespace tui {

void test_clipboard_thread_safety() {
    std::cout << "Testing thread-safe clipboard..." << std::endl;
    
    TextInput input("initial text");
    input.select_all();
    
    const int num_threads = 10;
    std::vector<std::thread> threads;
    
    // Multiple threads writing to clipboard simultaneously
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&input, i]() {
            std::string test_text = "Thread " + std::to_string(i);
            input.set_value(test_text);
            input.select_all();
            input.copy_to_clipboard();
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Verify clipboard has some value (no crash = success)
    std::cout << "✓ Clipboard thread safety test passed" << std::endl;
}

void test_clipboard_concurrent_read_write() {
    std::cout << "Testing concurrent clipboard read/write..." << std::endl;
    
    TextInput input("test data");
    input.select_all();
    
    std::vector<std::thread> threads;
    bool error = false;
    
    // Writers
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&input, &error, i]() {
            try {
                for (int j = 0; j < 100; j++) {
                    input.set_value("Writer" + std::to_string(i) + "_" + std::to_string(j));
                    input.select_all();
                    input.copy_to_clipboard();
                }
            } catch (...) {
                error = true;
            }
        });
    }
    
    // Readers
    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&input, &error]() {
            try {
                for (int j = 0; j < 100; j++) {
                    input.paste_from_clipboard();
                }
            } catch (...) {
                error = true;
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    assert(!error && "Exception occurred during concurrent access");
    std::cout << "✓ Concurrent read/write test passed" << std::endl;
}

} // namespace tui

namespace tui {

void run_thread_safety_tests() {
    std::cout << "=== Thread Safety Tests ===" << std::endl;
    test_clipboard_thread_safety();
    test_clipboard_concurrent_read_write();
    std::cout << "All thread safety tests passed!" << std::endl;
}

} // namespace tui
