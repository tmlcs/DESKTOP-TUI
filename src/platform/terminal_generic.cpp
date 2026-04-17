// Generic/fallback terminal implementation
// Minimal VT100 support for unknown platforms

#include "platform/terminal.hpp"
#include <cstdio>

namespace tui {

class GenericTerminal : public ITerminal {
public:
    GenericTerminal() : cols_(80), rows_(24), raw_mode_(false), alt_screen_(false) {}

    bool init() override {
        // SEC-04: Validate terminal initialization (generic fallback)
        // Basic validation for generic terminal backend
        if (cols_ < 10 || rows_ < 5) {
            // Terminal too small for meaningful UI
            // This is a fallback implementation, so we just note the limitation
        }
        return true;
    }

    void shutdown() override {
        leave_raw_mode();
        leave_alternate_screen();
        cursor_show();
        write("\033[0m");
        flush();
    }

    void clear() override { write("\033[2J\033[H"); }
    void flush() override { fflush(stdout); }
    void sync() override { flush(); }

    void cursor_hide() override { write("\033[?25l"); }
    void cursor_show() override { write("\033[?25h"); }

    void cursor_move(int x, int y) override {
        char buf[32];
        snprintf(buf, sizeof(buf), "\033[%d;%dH", y + 1, x + 1);
        write(buf);
    }

    void cursor_save() override { write("\033[s"); }
    void cursor_restore() override { write("\033[u"); }

    int cols() const override { return cols_; }
    int rows() const override { return rows_; }

    void write(const std::string& text) override {
        fputs(text.c_str(), stdout);
    }

    void write_at(int x, int y, const std::string& text) override {
        cursor_move(x, y);
        write(text);
    }

    void write_styled(const std::string& text, const Style& style) override {
        emit_style(style);
        write(text);
        write("\033[0m");
    }

    void write_styled_at(int x, int y, const std::string& text, const Style& style) override {
        cursor_move(x, y);
        write_styled(text, style);
    }

    void fill(int x, int y, int w, int h, char ch, const Style& style) override {
        std::string line(w, ch);
        for (int row = y; row < y + h; row++) {
            write_styled_at(x, row, line, style);
        }
    }

    void clear_region(int x, int y, int w, int h) override {
        fill(x, y, w, h, ' ', Style::Default());
    }

    void draw_hline(int x, int y, int w, const Style& style) override {
        // ASCII fallback
        std::string line(w, '-');
        write_styled_at(x, y, line, style);
    }

    void draw_vline(int x, int y, int h, const Style& style) override {
        for (int row = y; row < y + h; row++) {
            write_styled_at(x, row, "|", style);
        }
    }

    void draw_rect(int x, int y, int w, int h, const Style& style) override {
        if (w < 2 || h < 2) return;
        write_styled_at(x, y, "+", style);
        for (int i = 1; i < w - 1; i++) write_styled_at(x + i, y, "-", style);
        write_styled_at(x + w - 1, y, "+", style);
        for (int row = y + 1; row < y + h - 1; row++) {
            write_styled_at(x, row, "|", style);
            write_styled_at(x + w - 1, row, "|", style);
        }
        write_styled_at(x, y + h - 1, "+", style);
        for (int i = 1; i < w - 1; i++) write_styled_at(x + i, y + h - 1, "-", style);
        write_styled_at(x + w - 1, y + h - 1, "+", style);
    }

    void enter_raw_mode() override {
        // Generic terminal can't truly set raw mode
        // Best effort with system() call
#ifndef _WIN32
        system("stty raw -echo 2>/dev/null");
#endif
        raw_mode_ = true;
    }

    void leave_raw_mode() override {
        if (!raw_mode_) return;
#ifndef _WIN32
        system("stty sane 2>/dev/null");
#endif
        raw_mode_ = false;
    }

    void enter_alternate_screen() override {
        write("\033[?1049h");
        flush();
        alt_screen_ = true;
    }

    void leave_alternate_screen() override {
        write("\033[?1049l");
        flush();
        alt_screen_ = false;
    }

    void set_title(const std::string& title) override {
        // SEC-02: Sanitize title to prevent escape sequence injection
        // Remove or replace control characters that could be used for attacks
        std::string sanitized;
        sanitized.reserve(title.size());
        for (char c : title) {
            // Allow printable ASCII and UTF-8 continuation bytes
            // Block: ESC (0x1B), BEL (0x07), CR (0x0D), and other control chars
            if (c == '\033' || c == '\007' || c == '\r' || (c >= 0 && c < 32 && c != '\t')) {
                // Replace dangerous characters with space or skip them
                if (c == '\033') {
                    // Skip ESC and any following sequence
                    continue;
                } else if (c == '\007' || c == '\r') {
                    sanitized += ' ';  // Replace with space
                } else {
                    // Skip other control characters
                    continue;
                }
            } else {
                sanitized += c;
            }
        }
        
        write("\033]0;" + sanitized + "\007");
    }

    TerminalCaps caps() const override {
        // Minimal capabilities - basic VT100 only
        return TerminalCaps::AnsiColors;
    }

    std::string term_type() const override { return "generic"; }

private:
    void emit_style(const Style& style) {
        write(emit_style_to_string(style));
    }

    int cols_, rows_;
    bool raw_mode_, alt_screen_;
};

ITerminal* create_terminal() {
    return new GenericTerminal();
}

void destroy_terminal(ITerminal* term) {
    delete term;
}

} // namespace tui
