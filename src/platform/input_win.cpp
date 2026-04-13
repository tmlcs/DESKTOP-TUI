// Windows input handler using ReadConsoleInput

#include "input.hpp"

#ifdef TUI_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace tui {

class WindowsInput : public IInput {
public:
    WindowsInput() : mouse_x_(0), mouse_y_(0) {}

    bool init() override { return true; }

    void shutdown() override {}

    bool has_input() override {
        DWORD num_events = 0;
        GetNumberOfConsoleInputEvents(hIn_, &num_events);
        return num_events > 0;
    }

    std::optional<Event> poll() override {
        INPUT_RECORD record;
        DWORD read = 0;
        ReadConsoleInput(hIn_, &record, 1, &read);
        if (read == 0) return std::nullopt;

        if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
            return process_key(record.Event.KeyEvent);
        }
        if (record.EventType == MOUSE_EVENT) {
            return process_mouse(record.Event.MouseEvent);
        }
        if (record.EventType == WINDOW_BUFFER_SIZE_EVENT) {
            Event e(EventType::Resize);
            e.cols = record.Event.WindowBufferSizeEvent.dwSize.X;
            e.rows = record.Event.WindowBufferSizeEvent.dwSize.Y;
            return e;
        }

        return std::nullopt;
    }

    int mouse_x() const override { return mouse_x_; }
    int mouse_y() const override { return mouse_y_; }

private:
    std::optional<Event> process_key(const KEY_EVENT_RECORD& ker) {
        Event e(EventType::KeyPress);
        e.key_code = ker.uChar.UnicodeChar;

        if (ker.wVirtualKeyCode == VK_ESCAPE) {
            e.key_code = Keys::Escape; e.key_name = "Escape";
        } else if (ker.wVirtualKeyCode == VK_RETURN) {
            e.key_code = Keys::Enter; e.key_name = "Enter";
        } else if (ker.wVirtualKeyCode == VK_TAB) {
            e.key_code = Keys::Tab; e.key_name = "Tab";
        } else if (ker.wVirtualKeyCode == VK_BACK) {
            e.key_code = Keys::Backspace; e.key_name = "Backspace";
        } else if (ker.wVirtualKeyCode == VK_UP) {
            e.key_code = Keys::ArrowUp; e.key_name = "ArrowUp";
        } else if (ker.wVirtualKeyCode == VK_DOWN) {
            e.key_code = Keys::ArrowDown; e.key_name = "ArrowDown";
        } else if (ker.wVirtualKeyCode == VK_LEFT) {
            e.key_code = Keys::ArrowLeft; e.key_name = "ArrowLeft";
        } else if (ker.wVirtualKeyCode == VK_RIGHT) {
            e.key_code = Keys::ArrowRight; e.key_name = "ArrowRight";
        } else if (ker.wVirtualKeyCode >= VK_F1 && ker.wVirtualKeyCode <= VK_F12) {
            e.key_code = Keys::F1 + (ker.wVirtualKeyCode - VK_F1);
            e.key_name = "F" + std::to_string(ker.wVirtualKeyCode - VK_F1 + 1);
        } else if (ker.wVirtualKeyCode == VK_HOME) {
            e.key_code = Keys::Home; e.key_name = "Home";
        } else if (ker.wVirtualKeyCode == VK_END) {
            e.key_code = Keys::End; e.key_name = "End";
        } else if (ker.wVirtualKeyCode == VK_PRIOR) {
            e.key_code = Keys::PageUp; e.key_name = "PageUp";
        } else if (ker.wVirtualKeyCode == VK_NEXT) {
            e.key_code = Keys::PageDown; e.key_name = "PageDown";
        } else if (ker.wVirtualKeyCode == VK_DELETE) {
            e.key_code = Keys::Delete_; e.key_name = "Delete";
        } else if (ker.wVirtualKeyCode == VK_INSERT) {
            e.key_code = Keys::Insert; e.key_name = "Insert";
        } else if (e.key_code >= 32 && e.key_code < 127) {
            e.key_name = std::string(1, (char)e.key_code);
        }

        if (ker.dwControlKeyState & LEFT_CTRL_PRESSED || ker.dwControlKeyState & RIGHT_CTRL_PRESSED)
            e.mods.control = true;
        if (ker.dwControlKeyState & LEFT_ALT_PRESSED || ker.dwControlKeyState & RIGHT_ALT_PRESSED)
            e.mods.alt = true;
        if (ker.dwControlKeyState & SHIFT_PRESSED)
            e.mods.shift = true;

        return e;
    }

    std::optional<Event> process_mouse(const MOUSE_EVENT_RECORD& mer) {
        Event e;
        mouse_x_ = mer.dwMousePosition.X;
        mouse_y_ = mer.dwMousePosition.Y;
        e.mouse_x = mouse_x_;
        e.mouse_y = mouse_y_;

        if (mer.dwEventFlags == MOUSE_MOVED) {
            e.type = EventType::MouseMove;
            return e;
        }
        if (mer.dwEventFlags == MOUSE_WHEELED) {
            e.type = EventType::MouseScroll;
            e.scroll_delta = (mer.dwButtonState >> 16) > 0 ? 1 : -1;
            return e;
        }

        e.type = EventType::MouseDown;
        if (mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) e.mouse_button = 0;
        else if (mer.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) e.mouse_button = 1;
        else if (mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED) e.mouse_button = 2;

        return e;
    }

    HANDLE hIn_ = GetStdHandle(STD_INPUT_HANDLE);
    int mouse_x_, mouse_y_;
};

IInput* create_input() {
    return new WindowsInput();
}

void destroy_input(IInput* input) {
    delete input;
}

} // namespace tui

#endif // TUI_PLATFORM_WINDOWS
