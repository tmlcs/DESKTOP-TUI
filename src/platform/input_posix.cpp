// POSIX (Linux, macOS) input handler
// Uses termios raw mode + stdin reading

#include "input.hpp"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <cstring>
#include <vector>

namespace tui {

class PosixInput : public IInput {
public:
    PosixInput() : mouse_x_(0), mouse_y_(0) {}

    bool init() override {
        // Set stdin to non-blocking
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
        return true;
    }

    void shutdown() override {
        // Restore blocking mode
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }

    bool has_input() override {
        fd_set fds;
        struct timeval tv = {0, 0};
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        return select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0;
    }

    std::optional<Event> poll() override {
        unsigned char buf[16];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) return std::nullopt;

        // Parse escape sequences
        if (buf[0] == '\033') {
            if (n == 1) {
                // Lone escape = Escape key
                Event e(EventType::KeyPress);
                e.key_code = Keys::Escape;
                e.key_name = "Escape";
                return e;
            }
            if (buf[1] == '[') {
                return parse_csi(buf, n);
            }
            if (buf[1] == 'O') {
                return parse_ss3(buf, n);
            }
            if (buf[1] >= ' ' && buf[1] <= '~') {
                // Alt+key
                Event e(EventType::KeyPress);
                e.key_code = buf[1];
                e.key_name = std::string(1, (char)buf[1]);
                e.mods.alt = true;
                return e;
            }
        }

        // Regular character
        Event e(EventType::KeyPress);
        e.key_code = buf[0];
        if (buf[0] == '\r' || buf[0] == '\n') {
            e.key_code = Keys::Enter;
            e.key_name = "Enter";
        } else if (buf[0] == '\t') {
            e.key_code = Keys::Tab;
            e.key_name = "Tab";
        } else if (buf[0] == '\b' || buf[0] == 127) {
            e.key_code = Keys::Backspace;
            e.key_name = "Backspace";
        } else if (buf[0] < 32) {
            // Control character
            e.key_code = buf[0] + '@';  // Ctrl+A = 'A', etc.
            e.mods.control = true;
            e.key_name = std::string(1, (char)e.key_code);
        } else {
            e.key_name = std::string(1, (char)buf[0]);
        }
        return e;
    }

    int mouse_x() const override { return mouse_x_; }
    int mouse_y() const override { return mouse_y_; }

private:
    std::optional<Event> parse_csi(const unsigned char* buf, ssize_t n) {
        Event e;

        if (n >= 4 && buf[2] == '<') {
            // SGR 1006 mouse: ESC [ < button ; x ; y M/m
            int button, x, y;
            char last;
            if (sscanf((const char*)buf + 3, "%d;%d;%d%c", &button, &x, &y, &last) == 4) {
                mouse_x_ = x - 1;
                mouse_y_ = y - 1;

                if (last == 'M') {
                    e.type = EventType::MouseDown;
                    e.mouse_button = button & 3;
                    e.mods.shift = button & 4;
                    e.mods.meta = button & 8;
                    e.mods.control = button & 16;
                } else if (last == 'm') {
                    e.type = EventType::MouseUp;
                    e.mouse_button = button & 3;
                } else {
                    e.type = EventType::MouseMove;
                    e.mouse_button = button & 3;
                }
                e.mouse_x = mouse_x_;
                e.mouse_y = mouse_y_;
                return e;
            }
        }

        if (n == 3 && buf[2] >= 'A' && buf[2] <= 'D') {
            // Arrow keys
            e.type = EventType::KeyPress;
            switch (buf[2]) {
                case 'A': e.key_code = Keys::ArrowUp; e.key_name = "ArrowUp"; break;
                case 'B': e.key_code = Keys::ArrowDown; e.key_name = "ArrowDown"; break;
                case 'C': e.key_code = Keys::ArrowRight; e.key_name = "ArrowRight"; break;
                case 'D': e.key_code = Keys::ArrowLeft; e.key_name = "ArrowLeft"; break;
            }
            return e;
        }

        // Handle more CSI sequences (Home, End, F1-F5, Delete, Insert, PageUp/Down)
        if (n == 3) {
            switch (buf[2]) {
                case 'H': e.type = EventType::KeyPress; e.key_code = Keys::Home; e.key_name = "Home"; return e;
                case 'F': e.type = EventType::KeyPress; e.key_code = Keys::End; e.key_name = "End"; return e;
                case 'Z': e.type = EventType::KeyPress; e.key_code = Keys::Tab; e.key_name = "Tab"; e.mods.shift = true; return e;
            }
        }

        // Extended sequences: ESC [ 1 ; X ~ for function keys with modifiers
        if (n >= 5 && buf[n-1] == '~') {
            int code;
            sscanf((const char*)buf + 2, "%d", &code);
            e.type = EventType::KeyPress;
            switch (code) {
                case 1: case 7: e.key_code = Keys::Home; e.key_name = "Home"; break;
                case 2: e.key_code = Keys::Insert; e.key_name = "Insert"; break;
                case 3: e.key_code = Keys::Delete_; e.key_name = "Delete"; break;
                case 4: case 8: e.key_code = Keys::End; e.key_name = "End"; break;
                case 5: e.key_code = Keys::PageUp; e.key_name = "PageUp"; break;
                case 6: e.key_code = Keys::PageDown; e.key_name = "PageDown"; break;
                case 11: e.key_code = Keys::F1; e.key_name = "F1"; break;
                case 12: e.key_code = Keys::F2; e.key_name = "F2"; break;
                case 13: e.key_code = Keys::F3; e.key_name = "F3"; break;
                case 14: e.key_code = Keys::F4; e.key_name = "F4"; break;
                case 15: e.key_code = Keys::F5; e.key_name = "F5"; break;
                case 17: e.key_code = Keys::F6; e.key_name = "F6"; break;
                case 18: e.key_code = Keys::F7; e.key_name = "F7"; break;
                case 19: e.key_code = Keys::F8; e.key_name = "F8"; break;
                case 20: e.key_code = Keys::F9; e.key_name = "F9"; break;
                case 21: e.key_code = Keys::F10; e.key_name = "F10"; break;
                default: e.key_code = buf[0]; e.key_name = "Unknown"; break;
            }
            return e;
        }

        // Fallback
        e.type = EventType::KeyPress;
        e.key_code = buf[0];
        e.key_name = "Unknown";
        return e;
    }

    std::optional<Event> parse_ss3(const unsigned char* buf, ssize_t n) {
        Event e;
        e.type = EventType::KeyPress;
        if (n == 3) {
            switch (buf[2]) {
                case 'P': e.key_code = Keys::F1; e.key_name = "F1"; return e;
                case 'Q': e.key_code = Keys::F2; e.key_name = "F2"; return e;
                case 'R': e.key_code = Keys::F3; e.key_name = "F3"; return e;
                case 'S': e.key_code = Keys::F4; e.key_name = "F4"; return e;
                case 'H': e.key_code = Keys::Home; e.key_name = "Home"; return e;
                case 'F': e.key_code = Keys::End; e.key_name = "End"; return e;
            }
        }
        e.key_code = buf[0];
        e.key_name = "Unknown";
        return e;
    }

    int mouse_x_, mouse_y_;
};

IInput* create_input() {
    return new PosixInput();
}

void destroy_input(IInput* input) {
    delete input;
}

} // namespace tui
