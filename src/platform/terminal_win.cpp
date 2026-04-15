// Windows terminal implementation using Console API
// Uses SetConsoleMode, WriteConsoleOutput, ReadConsoleInput

#include "terminal.hpp"
#include "core/string_utils.hpp"

#ifdef TUI_PLATFORM_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <fcntl.h>

namespace tui {

class WindowsTerminal : public ITerminal {
public:
    WindowsTerminal() : raw_mode_(false), alt_screen_(false) {
        hOut_ = GetStdHandle(STD_OUTPUT_HANDLE);
        hIn_ = GetStdHandle(STD_INPUT_HANDLE);

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(hOut_, &csbi)) {
            cols_ = csbi.dwSize.X;
            rows_ = csbi.dwSize.Y;
        } else {
            cols_ = 80;
            rows_ = 24;
        }

        // Windows 10+ supports VT sequences via ENABLE_VIRTUAL_TERMINAL_PROCESSING
        DWORD mode = 0;
        if (GetConsoleMode(hOut_, &mode)) {
            supports_vt_ = SetConsoleMode(hOut_, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
        }
    }

    bool init() override { return true; }

    void shutdown() override {
        if (raw_mode_) leave_raw_mode();
        if (alt_screen_) leave_alternate_screen();
        if (supports_vt_) write("\033[0m");
        cursor_show();
        flush();
    }

    void clear() override {
        if (supports_vt_) {
            write("\033[2J\033[H");
        } else {
            COORD pos = {0, 0};
            DWORD written;
            FillConsoleOutputCharacterA(hOut_, ' ', cols_ * rows_, pos, &written);
            FillConsoleOutputAttribute(hOut_, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
                                      cols_ * rows_, pos, &written);
            SetConsoleCursorPosition(hOut_, pos);
        }
    }

    void flush() override { fflush(stdout); }
    void sync() override { flush(); }

    void cursor_hide() override {
        if (supports_vt_) write("\033[?25l");
        else { CONSOLE_CURSOR_INFO ci = {0}; ci.dwSize = 100; SetConsoleCursorInfo(hOut_, &ci); }
    }

    void cursor_show() override {
        if (supports_vt_) write("\033[?25h");
        else { CONSOLE_CURSOR_INFO ci = {100}; ci.dwSize = 100; SetConsoleCursorInfo(hOut_, &ci); }
    }

    void cursor_move(int x, int y) override {
        if (supports_vt_) {
            char buf[32];
            snprintf(buf, sizeof(buf), "\033[%d;%dH", y + 1, x + 1);
            write(buf);
        } else {
            COORD pos = {(SHORT)x, (SHORT)y};
            SetConsoleCursorPosition(hOut_, pos);
        }
    }

    void cursor_save() override { if (supports_vt_) write("\033[s"); }
    void cursor_restore() override { if (supports_vt_) write("\033[u"); }

    int cols() const override { return cols_; }
    int rows() const override { return rows_; }

    void write(const std::string& text) override {
        if (supports_vt_) {
            printf("%s", text.c_str());
        } else {
            // Fallback: write directly to console buffer
            DWORD written;
            WriteConsoleA(hOut_, text.c_str(), (DWORD)text.size(), &written, nullptr);
        }
    }

    void write_at(int x, int y, const std::string& text) override {
        cursor_move(x, y);
        write(text);
    }

    void write_styled(const std::string& text, const Style& style) override {
        if (supports_vt_) {
            emit_style(style);
            write(text);
            write("\033[0m");
        } else {
            // Simplified: just use default style
            write(text);
        }
    }

    void write_styled_at(int x, int y, const std::string& text, const Style& style) override {
        cursor_move(x, y);
        write_styled(text, style);
    }

    void fill(int x, int y, int w, int h, char ch, const Style& style) override {
        if (!supports_vt_) {
            COORD pos = {(SHORT)x, (SHORT)y};
            DWORD written;
            // Map Style to Windows console attributes
            WORD attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // default white
            if (style.fg.mode == Color::Mode::TrueColor) {
                if (style.fg.rgb.r > 128) attr |= FOREGROUND_RED;
                if (style.fg.rgb.g > 128) attr |= FOREGROUND_GREEN;
                if (style.fg.rgb.b > 128) attr |= FOREGROUND_BLUE;
            }
            if (style.bold) attr |= FOREGROUND_INTENSITY;
            for (int row = 0; row < h; row++) {
                FillConsoleOutputCharacterA(hOut_, ch, w, pos, &written);
                FillConsoleOutputAttribute(hOut_, attr, w, pos, &written);
                pos.Y++;
            }
            return;
        }
        std::string line(w, ch);
        for (int row = y; row < y + h; row++) {
            write_styled_at(x, row, line, style);
        }
    }

    void clear_region(int x, int y, int w, int h) override {
        fill(x, y, w, h, ' ', Style::Default());
    }

    void draw_hline(int x, int y, int w, const Style& style) override {
        const char* hline = supports_vt_ ? "\xe2\x94\x80" : "-";
        std::string line;
        line.reserve(supports_vt_ ? w * 3 : w);
        for (int i = 0; i < w; i++) line += hline;
        write_styled_at(x, y, line, style);
    }

    void draw_vline(int x, int y, int h, const Style& style) override {
        const char* v = supports_vt_ ? "\xe2\x94\x82" : "|";
        for (int row = y; row < y + h; row++) {
            write_styled_at(x, row, v, style);
        }
    }

    void draw_rect(int x, int y, int w, int h, const Style& style) override {
        if (w < 2 || h < 2) return;
        const char *tl, *tr, *bl, *br, *hline, *vline;
        if (supports_vt_) {
            tl = "\xe2\x94\x8c"; tr = "\xe2\x94\x90";
            bl = "\xe2\x94\x94"; br = "\xe2\x94\x98";
            hline = "\xe2\x94\x80"; vline = "\xe2\x94\x82";
        } else {
            tl = tr = bl = br = "+"; hline = "-"; vline = "|";
        }
        write_styled_at(x, y, tl, style);
        for (int i = 1; i < w - 1; i++) write_styled_at(x + i, y, hline, style);
        write_styled_at(x + w - 1, y, tr, style);
        for (int row = y + 1; row < y + h - 1; row++) {
            write_styled_at(x, row, vline, style);
            write_styled_at(x + w - 1, row, vline, style);
        }
        write_styled_at(x, y + h - 1, bl, style);
        for (int i = 1; i < w - 1; i++) write_styled_at(x + i, y + h - 1, hline, style);
        write_styled_at(x + w - 1, y + h - 1, br, style);
    }

    void enter_raw_mode() override {
        if (raw_mode_) return;
        DWORD mode = 0;
        GetConsoleMode(hIn_, &mode);
        orig_input_mode_ = mode;
        // Combine raw mode flags with window/mouse input in a single call
        SetConsoleMode(hIn_,
            (mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT))
            | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

        GetConsoleMode(hOut_, &mode);
        orig_output_mode_ = mode;
        if (supports_vt_) {
            SetConsoleMode(hOut_, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
        }

        if (supports_vt_) write("\033[?1006;1000;1015;2004h");
        raw_mode_ = true;
    }

    void leave_raw_mode() override {
        if (!raw_mode_) return;
        if (supports_vt_) write("\033[?1006;1000;1015;2004l");
        SetConsoleMode(hIn_, orig_input_mode_);
        SetConsoleMode(hOut_, orig_output_mode_);
        raw_mode_ = false;
    }

    void enter_alternate_screen() override {
        if (supports_vt_) {
            write("\033[?1049h");
            flush();
        }
        alt_screen_ = true;
    }

    void leave_alternate_screen() override {
        if (supports_vt_) {
            write("\033[?1049l");
            flush();
        }
        alt_screen_ = false;
    }

    void set_title(const std::string& title) override {
        SetConsoleTitleA(title.c_str());
    }

    TerminalCaps caps() const override {
        auto c = TerminalCaps::AnsiColors | TerminalCaps::Unicode | TerminalCaps::Mouse;
        if (supports_vt_) {
            c = c | TerminalCaps::Color256 | TerminalCaps::TrueColor |
                TerminalCaps::BoxDrawing | TerminalCaps::Resize;
        }
        return c;
    }

    std::string term_type() const override { return "windows-console"; }

private:
    void emit_style(const Style& style) {
        write(emit_style_to_string(style));
    }

    HANDLE hOut_, hIn_;
    int cols_, rows_;
    bool raw_mode_, alt_screen_, supports_vt_;
    DWORD orig_input_mode_, orig_output_mode_;
};

ITerminal* create_terminal() {
    return new WindowsTerminal();
}

void destroy_terminal(ITerminal* term) {
    delete term;
}

} // namespace tui

#endif // TUI_PLATFORM_WINDOWS
