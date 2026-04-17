// POSIX (Linux, macOS) input handler
// Uses termios raw mode + stdin reading

#include "platform/input.hpp"
#include "core/config.hpp"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <cstring>
#include <vector>

namespace tui {

class PosixInput : public IInput {
public:
    PosixInput() : mouse_x_(0), mouse_y_(0) {
        // Initialize with reasonable buffer capacity limit (configurable max)
        buffer_.reserve(Config::INPUT_BUFFER_INITIAL_RESERVE);
        max_buffer_size_ = Config::MAX_INPUT_BUFFER_SIZE; // Prevent DoS attacks
    }

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
        // If we have buffered bytes from a previous partial read, try to complete them
        if (!buffer_.empty()) {
            // Check buffer limit before reading more (prevent DoS)
            if (buffer_.size() >= max_buffer_size_) {
                // Buffer overflow - discard oldest data and start fresh
                buffer_.clear();
                Event e(EventType::KeyPress);
                e.key_code = Keys::Escape;
                e.key_name = "Escape";
                return e;
            }
            
            // Try to read more to complete the sequence
            unsigned char more[16];
            ssize_t n = read(STDIN_FILENO, more, sizeof(more));
            if (n > 0) {
                // Ensure we don't exceed max buffer size
                size_t available = max_buffer_size_ - buffer_.size();
                size_t to_copy = (size_t)n < available ? (size_t)n : available;
                buffer_.insert(buffer_.end(), more, more + to_copy);
            }
            // Try to parse the buffer
            auto result = try_parse_buffer();
            if (result.has_value()) {
                return result;
            }
            // If still incomplete after adding more data, return nullopt
            // The next poll() will try again
            if (n > 0) return std::nullopt;
            // No more data available, parse what we have
            auto fallback = try_parse_buffer();
            if (fallback.has_value()) return fallback;
            // Buffer is stale/incomplete, clear it and return escape
            buffer_.clear();
            Event e(EventType::KeyPress);
            e.key_code = Keys::Escape;
            e.key_name = "Escape";
            return e;
        }

        unsigned char buf[32];
        ssize_t n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n == 0) {
            // EOF — terminal closed, signal quit
            Event e(EventType::Quit);
            return e;
        }
        if (n < 0) return std::nullopt; // error or no data

        // Try to parse; if incomplete, buffer the bytes
        buffer_.assign(buf, buf + n);
        auto result = try_parse_buffer();
        if (result.has_value()) {
            return result;
        }

        // Incomplete sequence — save to buffer for next poll
        // This handles fragmented input
        auto fallback = try_parse_buffer();
        if (fallback.has_value()) return fallback;

        // Truly incomplete, return escape as fallback
        buffer_.clear();
        Event e(EventType::KeyPress);
        e.key_code = Keys::Escape;
        e.key_name = "Escape";
        return e;
    }

    int mouse_x() const override { return mouse_x_; }
    int mouse_y() const override { return mouse_y_; }

private:
    /// Try to parse the current buffer. Returns event if complete.
    /// Clears consumed bytes from buffer on success.
    std::optional<Event> try_parse_buffer() {
        if (buffer_.empty()) return std::nullopt;

        if (buffer_[0] == '\033') {
            if (buffer_.size() == 1) {
                // Incomplete: could be lone escape or start of sequence
                return std::nullopt; // wait for more
            }
            if (buffer_[1] == '[') {
                auto result = try_parse_csi();
                if (result.has_value()) return result;
                // CSI sequence might be incomplete
                return std::nullopt;
            }
            if (buffer_[1] == 'O') {
                auto result = try_parse_ss3();
                if (result.has_value()) return result;
                return std::nullopt;
            }
            if (buffer_[1] >= ' ' && buffer_[1] <= '~') {
                // Alt+key (complete)
                Event e(EventType::KeyPress);
                e.key_code = buffer_[1];
                e.key_name = std::string(1, (char)buffer_[1]);
                e.mods.alt = true;
                buffer_.erase(buffer_.begin(), buffer_.begin() + 2);
                return e;
            }
            // Unknown escape sequence, skip it
            buffer_.erase(buffer_.begin());
            return std::nullopt;
        }

        // Regular character
        Event e(EventType::KeyPress);
        e.key_code = buffer_[0];
        if (buffer_[0] == '\r' || buffer_[0] == '\n') {
            e.key_code = Keys::Enter;
            e.key_name = "Enter";
        } else if (buffer_[0] == '\t') {
            e.key_code = Keys::Tab;
            e.key_name = "Tab";
        } else if (buffer_[0] == '\b' || buffer_[0] == 127) {
            e.key_code = Keys::Backspace;
            e.key_name = "Backspace";
        } else if (buffer_[0] < 32) {
            e.key_code = buffer_[0] + '@';
            e.mods.control = true;
            e.key_name = std::string(1, (char)e.key_code);
        } else {
            e.key_name = std::string(1, (char)buffer_[0]);
        }
        buffer_.erase(buffer_.begin());
        return e;
    }

    std::optional<Event> try_parse_csi() {
        // Minimum CSI: ESC [ X  = 3 bytes
        if (buffer_.size() < 3) return std::nullopt;

        // SEC-01: Handle bracketed paste (ESC [ 2 0 0 ~ starts paste, ESC [ 2 0 1 ~ ends)
        // We treat all content between these markers as raw text, not commands
        if (buffer_.size() >= 6 && buffer_[2] == '2' && buffer_[3] == '0') {
            // Check for start (200~) or end (201~) of bracketed paste
            if (buffer_.size() >= 7 && buffer_[4] == '0' && buffer_[5] == '~') {
                // Start of bracketed paste: ESC[200~
                // Clear buffer and mark that we're in paste mode
                buffer_.erase(buffer_.begin(), buffer_.begin() + 6);
                in_bracketed_paste_ = true;
                return std::nullopt; // Consume the sequence, wait for paste content
            }
            if (buffer_.size() >= 7 && buffer_[4] == '1' && buffer_[5] == '~') {
                // End of bracketed paste: ESC[201~
                buffer_.erase(buffer_.begin(), buffer_.begin() + 6);
                in_bracketed_paste_ = false;
                return std::nullopt; // Consume the sequence
            }
        }

        // If we're inside bracketed paste, treat all input as raw text
        if (in_bracketed_paste_) {
            // Look for the end sequence ESC[201~
            size_t end_pos = 0;
            bool found_end = false;
            for (size_t i = 0; i + 6 <= buffer_.size(); i++) {
                if (buffer_[i] == '\033' && buffer_[i+1] == '[' &&
                    buffer_[i+2] == '2' && buffer_[i+3] == '0' &&
                    buffer_[i+4] == '1' && buffer_[i+5] == '~') {
                    end_pos = i;
                    found_end = true;
                    break;
                }
            }
            
            if (found_end) {
                // Extract paste content (everything before ESC[201~)
                std::string paste_content(buffer_.begin(), buffer_.begin() + static_cast<ptrdiff_t>(end_pos));
                // Remove the entire sequence including end marker
                buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<ptrdiff_t>(end_pos) + 6);
                
                // Create a custom event with the paste content
                Event e(EventType::Custom);
                e.data_s = paste_content;
                e.key_name = "BracketedPaste";
                in_bracketed_paste_ = false;
                return e;
            } else {
                // Incomplete paste - wait for more data
                return std::nullopt;
            }
        }

        // SGR 1006 mouse: ESC [ < button ; x ; y M/m
        if (buffer_.size() >= 4 && buffer_[2] == '<') {
            // Try to find the terminator ('M' or 'm')
            size_t term_pos = 0;
            bool found = false;
            for (size_t i = 3; i < buffer_.size(); i++) {
                if (buffer_[i] == 'M' || buffer_[i] == 'm') {
                    term_pos = i;
                    found = true;
                    break;
                }
            }
            if (!found) return std::nullopt; // incomplete

            Event e;
            int button, x, y;
            char last;
            std::string seq(buffer_.begin() + 3, buffer_.begin() + term_pos + 1);
            if (sscanf(seq.c_str(), "%d;%d;%d%c", &button, &x, &y, &last) == 4) {
                mouse_x_ = x - 1;
                mouse_y_ = y - 1;

                if (last == 'M') {
                    // Scroll wheel: buttons 64=up, 65=down, 66=left, 67=right
                    if (button >= 64 && button <= 67) {
                        e.type = EventType::MouseScroll;
                        switch (button) {
                            case 64: e.scroll_delta = -1; break; // scroll up
                            case 65: e.scroll_delta = 1; break;  // scroll down
                            case 66: e.scroll_delta = -2; break; // scroll left
                            case 67: e.scroll_delta = 2; break;  // scroll right
                        }
                    } else {
                        e.type = EventType::MouseDown;
                        e.mouse_button = button & 3;
                    }
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
                buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<ptrdiff_t>(term_pos) + 1);
                return e;
            }
            // Parse failed, skip this sequence
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<ptrdiff_t>(term_pos) + 1);
            return std::nullopt;
        }

        // Arrow keys: ESC [ A/B/C/D = 3 bytes
        if (buffer_.size() >= 3 && buffer_[2] >= 'A' && buffer_[2] <= 'D') {
            Event e;
            e.type = EventType::KeyPress;
            switch (buffer_[2]) {
                case 'A': e.key_code = Keys::ArrowUp; e.key_name = "ArrowUp"; break;
                case 'B': e.key_code = Keys::ArrowDown; e.key_name = "ArrowDown"; break;
                case 'C': e.key_code = Keys::ArrowRight; e.key_name = "ArrowRight"; break;
                case 'D': e.key_code = Keys::ArrowLeft; e.key_name = "ArrowLeft"; break;
            }
            buffer_.erase(buffer_.begin(), buffer_.begin() + 3);
            return e;
        }

        // Home/End/Shift-Tab: ESC [ H/F/Z = 3 bytes
        if (buffer_.size() >= 3) {
            switch (buffer_[2]) {
                case 'H': {
                    Event e; e.type = EventType::KeyPress; e.key_code = Keys::Home; e.key_name = "Home";
                    buffer_.erase(buffer_.begin(), buffer_.begin() + 3); return e;
                }
                case 'F': {
                    Event e; e.type = EventType::KeyPress; e.key_code = Keys::End; e.key_name = "End";
                    buffer_.erase(buffer_.begin(), buffer_.begin() + 3); return e;
                }
                case 'Z': {
                    Event e; e.type = EventType::KeyPress; e.key_code = Keys::Tab; e.key_name = "Tab";
                    e.mods.shift = true;
                    buffer_.erase(buffer_.begin(), buffer_.begin() + 3); return e;
                }
            }
        }

        // Extended: ESC [ 1 ; 2 ~ etc (need at least 6 bytes for "ESC[1;2~")
        // ESC [ N ~ where N is a number
        if (buffer_.size() >= 4 && buffer_[2] >= '0' && buffer_[2] <= '9') {
            // Find the terminator '~'
            size_t term_pos = 0;
            bool found = false;
            for (size_t i = 3; i < buffer_.size(); i++) {
                if (buffer_[i] == '~') {
                    term_pos = i;
                    found = true;
                    break;
                }
            }
            if (!found) return std::nullopt; // incomplete

            int code;
            std::string num_str(buffer_.begin() + 2, buffer_.begin() + static_cast<ptrdiff_t>(term_pos));
            if (sscanf(num_str.c_str(), "%d", &code) == 1) {
                Event e;
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
                    default: e.key_code = buffer_[0]; e.key_name = "Unknown"; break;
                }
                buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<ptrdiff_t>(term_pos) + 1);
                return e;
            }
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<ptrdiff_t>(term_pos) + 1);
            return std::nullopt;
        }

        return std::nullopt; // incomplete or unrecognized
    }

    std::optional<Event> try_parse_ss3() {
        // SS3: ESC O P/Q/R/S/H/F = 3 bytes
        if (buffer_.size() < 3) return std::nullopt;

        Event e;
        e.type = EventType::KeyPress;
        switch (buffer_[2]) {
            case 'P': e.key_code = Keys::F1; e.key_name = "F1"; break;
            case 'Q': e.key_code = Keys::F2; e.key_name = "F2"; break;
            case 'R': e.key_code = Keys::F3; e.key_name = "F3"; break;
            case 'S': e.key_code = Keys::F4; e.key_name = "F4"; break;
            case 'H': e.key_code = Keys::Home; e.key_name = "Home"; break;
            case 'F': e.key_code = Keys::End; e.key_name = "End"; break;
            default: return std::nullopt;
        }
        buffer_.erase(buffer_.begin(), buffer_.begin() + 3);
        return e;
    }

    std::vector<unsigned char> buffer_;
    size_t max_buffer_size_;
    int mouse_x_, mouse_y_;
    bool in_bracketed_paste_ = false;  // SEC-01: Track bracketed paste state
};

IInput* create_input() {
    return new PosixInput();
}

void destroy_input(IInput* input) {
    delete input;
}

} // namespace tui
