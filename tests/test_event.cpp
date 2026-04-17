// Tests for event.hpp - EventBus functionality
#include "core/event.hpp"
#include <cstdio>
#include <cstring>

namespace tui {

#define TEST(name, expr) do { \
    if (expr) { (*passed)++; printf("  PASS: %s\n", name); } \
    else { (*failed)++; printf("  FAIL: %s\n", name); } \
} while(0)

void test_event_basic(int* passed, int* failed) {
    printf("\n=== Event basic construction ===\n");

    Event e1;
    TEST("default event type", e1.type == EventType::Custom);
    TEST("default key_code", e1.key_code == 0);
    TEST("default mouse coords", e1.mouse_x == 0 && e1.mouse_y == 0);

    Event e2(EventType::KeyPress);
    TEST("explicit type constructor", e2.type == EventType::KeyPress);

    Event e3;
    e3.type = EventType::MouseScroll;
    e3.mouse_button = 3;
    e3.scroll_delta = 1;
    TEST("mouse scroll setup", e3.type == EventType::MouseScroll && 
                           e3.mouse_button == 3 && e3.scroll_delta == 1);
}

void test_event_key_mods(int* passed, int* failed) {
    printf("\n=== Event KeyMods ===\n");

    KeyMods mods;
    TEST("default mods false", !mods.shift && !mods.control && !mods.alt && !mods.meta);

    mods.shift = true;
    mods.control = true;
    TEST("shift and control set", mods.shift && mods.control);
}

void test_event_key_constants(int* passed, int* failed) {
    printf("\n=== Event key constants ===\n");

    TEST("Escape value", Keys::Escape == 0x100);
    TEST("Enter value", Keys::Enter == 0x101);
    TEST("Tab value", Keys::Tab == 0x102);
    TEST("Backspace value", Keys::Backspace == 0x103);
    TEST("ArrowUp value", Keys::ArrowUp == 0x10A);
    TEST("ArrowDown value", Keys::ArrowDown == 0x10B);
    TEST("F1 value", Keys::F1 == 0x110);
    TEST("F12 value", Keys::F12 == 0x11B);
}

void test_eventbus_subscribe_publish(int* passed, int* failed) {
    printf("\n=== EventBus subscribe/publish ===\n");

    EventBus bus;
    bool called = false;

    auto id = bus.subscribe(EventType::KeyPress, [&](const Event& e) {
        called = true;
        (void)e;
    });

    TEST("subscribe returns positive id", id > 0);

    Event e(EventType::KeyPress);
    bus.publish(e);
    TEST("handler called on publish", called);
}

void test_eventbus_multiple_subscribers(int* passed, int* failed) {
    printf("\n=== EventBus multiple subscribers ===\n");

    EventBus bus;
    int count = 0;

    bus.subscribe(EventType::KeyPress, [&](const Event&) { count++; });
    bus.subscribe(EventType::KeyPress, [&](const Event&) { count++; });
    bus.subscribe(EventType::KeyPress, [&](const Event&) { count++; });

    Event e(EventType::KeyPress);
    bus.publish(e);
    TEST("all handlers called", count == 3);
}

void test_eventbus_unsubscribe(int* passed, int* failed) {
    printf("\n=== EventBus unsubscribe ===\n");

    EventBus bus;
    int count = 0;

    auto id1 = bus.subscribe(EventType::KeyPress, [&](const Event&) { count++; });
    auto id2 = bus.subscribe(EventType::KeyPress, [&](const Event&) { count++; });
    auto id3 = bus.subscribe(EventType::KeyPress, [&](const Event&) { count++; });

    Event e(EventType::KeyPress);
    bus.publish(e);
    TEST("all handlers called initially", count == 3);

    bus.unsubscribe(id2);
    count = 0;
    bus.publish(e);
    TEST("unsubscribed handler not called", count == 2);

    bus.unsubscribe(id1);
    bus.unsubscribe(id3);
    count = 0;
    bus.publish(e);
    TEST("all unsubscribed", count == 0);
}

void test_eventbus_different_event_types(int* passed, int* failed) {
    printf("\n=== EventBus different event types ===\n");

    EventBus bus;
    int key_count = 0;
    int mouse_count = 0;

    bus.subscribe(EventType::KeyPress, [&](const Event&) { key_count++; });
    bus.subscribe(EventType::MouseDown, [&](const Event&) { mouse_count++; });

    Event key_event(EventType::KeyPress);
    Event mouse_event(EventType::MouseDown);

    bus.publish(key_event);
    TEST("key handler called", key_count == 1 && mouse_count == 0);

    bus.publish(mouse_event);
    TEST("mouse handler called", key_count == 1 && mouse_count == 1);
}

void test_eventbus_wildcard_subscription(int* passed, int* failed) {
    printf("\n=== EventBus wildcard subscription ===\n");

    EventBus bus;
    int wildcard_count = 0;
    int specific_count = 0;

    bus.subscribe_all([&](const Event&) { wildcard_count++; });
    bus.subscribe(EventType::KeyPress, [&](const Event&) { specific_count++; });

    Event key_event(EventType::KeyPress);
    bus.publish(key_event);
    TEST("both wildcard and specific called", wildcard_count == 1 && specific_count == 1);

    Event mouse_event(EventType::MouseDown);
    bus.publish(mouse_event);
    TEST("only wildcard for other events", wildcard_count == 2 && specific_count == 1);
}

void test_eventbus_self_unsubscribe(int* passed, int* failed) {
    printf("\n=== EventBus self-unsubscribe (snapshot test) ===\n");

    EventBus bus;
    int count = 0;
    EventBus::SubscriptionId id_to_remove = 0;

    auto id1 = bus.subscribe(EventType::KeyPress, [&](const Event&) {
        count++;
    });

    auto id2 = bus.subscribe(EventType::KeyPress, [&](const Event&) {
        count++;
        bus.unsubscribe(id_to_remove);
    });

    auto id3 = bus.subscribe(EventType::KeyPress, [&](const Event&) {
        count++;
    });

    id_to_remove = id1;
    Event e(EventType::KeyPress);
    bus.publish(e);

    // Should complete without crash, all 3 called in this emit
    TEST("self-unsubscribe doesn't crash", count == 3);

    // After unsubscribe, only 2 should be called
    count = 0;
    bus.publish(e);
    TEST("after self-unsubscribe, correct count", count == 2);
}

void test_eventbus_data_fields(int* passed, int* failed) {
    printf("\n=== Event data fields ===\n");

    Event e;
    e.data_i = 42;
    e.data_f = 3.14;
    e.data_s = "hello";

    TEST("integer data", e.data_i == 42);
    TEST("float data", e.data_f == 3.14);
    TEST("string data", e.data_s == "hello");
}

void test_eventbus_resize_event(int* passed, int* failed) {
    printf("\n=== Event resize data ===\n");

    Event e(EventType::Resize);
    e.cols = 80;
    e.rows = 24;

    TEST("resize cols", e.cols == 80);
    TEST("resize rows", e.rows == 24);
}

void test_eventbus_mouse_event(int* passed, int* failed) {
    printf("\n=== Event mouse data ===\n");

    Event e(EventType::MouseDown);
    e.mouse_x = 10;
    e.mouse_y = 20;
    e.mouse_button = 0;

    TEST("mouse x", e.mouse_x == 10);
    TEST("mouse y", e.mouse_y == 20);
    TEST("mouse button", e.mouse_button == 0);
}

void run_event_tests(int* passed, int* failed) {
    printf("Running event tests...\n");
    test_event_basic(passed, failed);
    test_event_key_mods(passed, failed);
    test_event_key_constants(passed, failed);
    test_eventbus_subscribe_publish(passed, failed);
    test_eventbus_multiple_subscribers(passed, failed);
    test_eventbus_unsubscribe(passed, failed);
    test_eventbus_different_event_types(passed, failed);
    test_eventbus_wildcard_subscription(passed, failed);
    test_eventbus_self_unsubscribe(passed, failed);
    test_eventbus_data_fields(passed, failed);
    test_eventbus_resize_event(passed, failed);
    test_eventbus_mouse_event(passed, failed);
}

} // namespace tui
