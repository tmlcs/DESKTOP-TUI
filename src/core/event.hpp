#ifndef TUI_CORE_EVENT_HPP
#define TUI_CORE_EVENT_HPP

#include <functional>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdint>
#include <string>

namespace tui {

enum class EventType {
    KeyPress,
    KeyRelease,
    MouseDown,
    MouseUp,
    MouseMove,
    MouseScroll,
    Resize,
    FocusIn,
    FocusOut,
    DesktopSwitch,
    WindowCreate,
    WindowClose,
    WindowFocus,
    WindowMove,
    WindowResize,
    Quit,
    Custom
};

struct KeyMods {
    bool shift   : 1;
    bool control : 1;
    bool alt     : 1;
    bool meta    : 1;

    KeyMods() : shift(false), control(false), alt(false), meta(false) {}
};

struct Event {
    EventType type;

    // Key data
    uint32_t key_code;    // Unicode codepoint or special key constant
    std::string key_name; // Human-readable name
    KeyMods mods;

    // Mouse data
    int mouse_x;
    int mouse_y;
    int mouse_button;   // 0=left, 1=middle, 2=right, 3=scroll_up, 4=scroll_down
    int scroll_delta;

    // Resize data
    int cols;
    int rows;

    // Custom data
    int64_t data_i;
    double data_f;
    std::string data_s;

    Event() : type(EventType::Custom), key_code(0), mouse_x(0), mouse_y(0),
              mouse_button(0), scroll_delta(0), cols(0), rows(0),
              data_i(0), data_f(0.0) {}

    explicit Event(EventType t) : type(t), key_code(0), mouse_x(0), mouse_y(0),
                                   mouse_button(0), scroll_delta(0), cols(0),
                                   rows(0), data_i(0), data_f(0.0) {}
};

// Special key codes (above Unicode range)
namespace Keys {
    constexpr uint32_t Escape   = 0x100;
    constexpr uint32_t Enter    = 0x101;
    constexpr uint32_t Tab      = 0x102;
    constexpr uint32_t Backspace= 0x103;
    constexpr uint32_t Insert   = 0x104;
    constexpr uint32_t Delete_  = 0x105;
    constexpr uint32_t Home     = 0x106;
    constexpr uint32_t End      = 0x107;
    constexpr uint32_t PageUp   = 0x108;
    constexpr uint32_t PageDown = 0x109;
    constexpr uint32_t ArrowUp  = 0x10A;
    constexpr uint32_t ArrowDown= 0x10B;
    constexpr uint32_t ArrowLeft= 0x10C;
    constexpr uint32_t ArrowRight = 0x10D;
    constexpr uint32_t F1       = 0x110;
    constexpr uint32_t F2       = 0x111;
    constexpr uint32_t F3       = 0x112;
    constexpr uint32_t F4       = 0x113;
    constexpr uint32_t F5       = 0x114;
    constexpr uint32_t F6       = 0x115;
    constexpr uint32_t F7       = 0x116;
    constexpr uint32_t F8       = 0x117;
    constexpr uint32_t F9       = 0x118;
    constexpr uint32_t F10      = 0x119;
    constexpr uint32_t F11      = 0x11A;
    constexpr uint32_t F12      = 0x11B;
}

// Event handler type
using EventHandler = std::function<void(const Event&)>;

// Event bus
class EventBus {
public:
    using SubscriptionId = uint64_t;

    SubscriptionId subscribe(EventType type, EventHandler handler) {
        auto id = next_id_++;
        handlers_[type].push_back({id, std::move(handler)});
        return id;
    }

    void unsubscribe(SubscriptionId id) {
        for (auto& [type, vec] : handlers_) {
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [id](const auto& h) { return h.id == id; }),
                vec.end()
            );
        }
    }

    void publish(const Event& event) {
        auto it = handlers_.find(event.type);
        if (it != handlers_.end()) {
            for (const auto& [id, handler] : it->second) {
                handler(event);
            }
        }
        // Also publish to wildcard handlers
        if (!wildcard_handlers_.empty()) {
            for (const auto& handler : wildcard_handlers_) {
                handler.callback(event);
            }
        }
    }

    SubscriptionId subscribe_all(EventHandler handler) {
        auto id = next_id_++;
        wildcard_handlers_.push_back({id, std::move(handler)});
        return id;
    }

private:
    struct Handler {
        SubscriptionId id;
        EventHandler callback;
    };

    uint64_t next_id_ = 1;
    std::map<EventType, std::vector<Handler>> handlers_;
    std::vector<Handler> wildcard_handlers_;
};

} // namespace tui

#endif // TUI_CORE_EVENT_HPP
