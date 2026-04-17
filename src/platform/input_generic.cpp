// Generic/fallback input handler
// Minimal stdin reading for unknown platforms

#include "platform/input.hpp"
#include <cstdio>
#include <vector>

namespace tui {

class GenericInput : public IInput {
public:
    GenericInput() : mouse_x_(0), mouse_y_(0) {}

    bool init() override { return true; }
    void shutdown() override {}

    bool has_input() override {
        // Generic terminal can't do non-blocking input reliably.
        // Return false so the main loop continues with rendering
        // and only blocks when there's actual data on stdin.
        return false;
    }

    std::optional<Event> poll() override {
        int ch = getchar();
        if (ch == EOF) return std::nullopt;

        Event e(EventType::KeyPress);
        e.key_code = ch;

        if (ch == '\033') {
            // Check for escape sequence
            int ch2 = getchar();
            if (ch2 == EOF) {
                e.key_code = Keys::Escape;
                e.key_name = "Escape";
                return e;
            }
            if (ch2 == '[') {
                int ch3 = getchar();
                if (ch3 >= 'A' && ch3 <= 'D') {
                    switch (ch3) {
                        case 'A': e.key_code = Keys::ArrowUp; e.key_name = "ArrowUp"; break;
                        case 'B': e.key_code = Keys::ArrowDown; e.key_name = "ArrowDown"; break;
                        case 'C': e.key_code = Keys::ArrowRight; e.key_name = "ArrowRight"; break;
                        case 'D': e.key_code = Keys::ArrowLeft; e.key_name = "ArrowLeft"; break;
                    }
                    return e;
                }
            }
            // Alt+key
            e.mods.alt = true;
            e.key_code = ch2;
            e.key_name = std::string(1, (char)ch2);
            return e;
        }

        if (ch == '\r' || ch == '\n') {
            e.key_code = Keys::Enter; e.key_name = "Enter";
        } else if (ch == '\t') {
            e.key_code = Keys::Tab; e.key_name = "Tab";
        } else if (ch == '\b' || ch == 127) {
            e.key_code = Keys::Backspace; e.key_name = "Backspace";
        } else if (ch >= 32) {
            e.key_name = std::string(1, (char)ch);
        }

        return e;
    }

    int mouse_x() const override { return mouse_x_; }
    int mouse_y() const override { return mouse_y_; }

private:
    int mouse_x_, mouse_y_;
};

IInput* create_input() {
    return new GenericInput();
}

void destroy_input(IInput* input) {
    delete input;
}

} // namespace tui
