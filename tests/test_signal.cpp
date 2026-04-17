// Tests for signal.hpp - Signal/Slot functionality
#include "core/signal.hpp"
#include <cstdio>
#include <cstring>

namespace tui {

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

void test_signal_basic(int* passed, int* failed) {
    printf("\n=== Signal basic operations ===\n");

    Signal<> sig;
    TEST("signal empty initially", sig.empty());

    bool called = false;
    auto id = sig.connect([&called]() { called = true; });
    TEST("signal not empty after connect", !sig.empty());

    sig.emit();
    TEST("callback called on emit", called);

    sig.disconnect(id);
    TEST("signal empty after disconnect", sig.empty());
}

void test_signal_with_args(int* passed, int* failed) {
    printf("\n=== Signal with arguments ===\n");

    Signal<int> sig;
    int received_int = 0;

    sig.connect([&](int i) {
        received_int = i;
    });

    sig.emit(42);
    TEST("int arg passed", received_int == 42);
}

void test_signal_multiple_slots(int* passed, int* failed) {
    printf("\n=== Signal multiple slots ===\n");

    Signal<int> sig;
    int sum = 0;

    sig.connect([&](int v) { sum += v; });
    sig.connect([&](int v) { sum += v * 2; });
    sig.connect([&](int v) { sum += v * 3; });

    sig.emit(10);
    TEST("all slots called", sum == 60); // 10 + 20 + 30
}

void test_signal_disconnect_specific(int* passed, int* failed) {
    printf("\n=== Signal disconnect specific slot ===\n");

    Signal<int> sig;
    int sum = 0;

    auto id1 = sig.connect([&](int v) { sum += v; });
    auto id2 = sig.connect([&](int v) { sum += v * 10; });
    auto id3 = sig.connect([&](int v) { sum += v * 100; });

    sig.emit(1);
    TEST("all slots called initially", sum == 111);

    sig.disconnect(id2);
    sum = 0;
    sig.emit(1);
    TEST("middle slot disconnected", sum == 101); // 1 + 100

    sig.disconnect(id1);
    sum = 0;
    sig.emit(1);
    TEST("first slot disconnected", sum == 100);

    sig.disconnect(id3);
    TEST("all slots disconnected", sig.empty());
}

void test_signal_emit_after_disconnect(int* passed, int* failed) {
    printf("\n=== Signal emit after disconnect ===\n");

    Signal<> sig;
    int count = 0;

    auto id1 = sig.connect([&]() { count++; });
    auto id2 = sig.connect([&]() { count++; });

    sig.emit();
    TEST("both called before disconnect", count == 2);

    sig.disconnect(id1);
    count = 0;
    sig.emit();
    TEST("only one called after disconnect", count == 1);
}

void test_signal_clear(int* passed, int* failed) {
    printf("\n=== Signal clear ===\n");

    Signal<> sig;
    int count = 0;

    sig.connect([&]() { count++; });
    sig.connect([&]() { count++; });
    sig.connect([&]() { count++; });

    sig.clear();
    TEST("signal empty after clear", sig.empty());

    sig.emit();
    TEST("no callbacks after clear", count == 0);
}

void test_signal_self_disconnect(int* passed, int* failed) {
    printf("\n=== Signal self-disconnect (snapshot test) ===\n");

    Signal<> sig;
    int count = 0;
    Signal<>::SlotId id_to_disconnect = 0;

    auto id1 = sig.connect([&]() { 
        count++; 
    });

    auto id2 = sig.connect([&]() { 
        count++; 
        sig.disconnect(id_to_disconnect);
    });

    auto id3 = sig.connect([&]() { 
        count++; 
    });

    id_to_disconnect = id1;
    sig.emit();
    
    // Should complete without crash, and all 3 should be called in this emit
    TEST("self-disconnect doesn't crash", count == 3);
    
    // After disconnect, only 2 should be called
    count = 0;
    sig.emit();
    TEST("after self-disconnect, correct count", count == 2);
}

void test_signal_copy_safety(int* passed, int* failed) {
    printf("\n=== Signal copy safety ===\n");

    Signal<int> sig;
    int sum = 0;

    sig.connect([&](int v) { sum += v; });
    sig.connect([&](int v) { sum += v * 2; });

    // Emit should use snapshot, so modifying during emit is safe
    sig.emit(5);
    TEST("emit result", sum == 15);
}

void test_signal_return_values(int* passed, int* failed) {
    printf("\n=== Signal return values ===\n");

    Signal<> sig;
    
    auto id1 = sig.connect([]() {});
    auto id2 = sig.connect([]() {});
    
    TEST("connect returns unique ids", id1 != id2);
    TEST("ids are positive", id1 > 0 && id2 > 0);
}

void run_signal_tests(int* passed, int* failed) {
    printf("Running signal tests...\n");
    test_signal_basic(passed, failed);
    test_signal_with_args(passed, failed);
    test_signal_multiple_slots(passed, failed);
    test_signal_disconnect_specific(passed, failed);
    test_signal_emit_after_disconnect(passed, failed);
    test_signal_clear(passed, failed);
    test_signal_self_disconnect(passed, failed);
    test_signal_copy_safety(passed, failed);
    test_signal_return_values(passed, failed);
}

} // namespace tui
