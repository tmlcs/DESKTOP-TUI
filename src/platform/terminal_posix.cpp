// POSIX (Linux, macOS, BSD) terminal implementation
// Uses termios for raw mode, ioctl for size, VT100 escape sequences

#include "terminal.hpp"
#include "core/string_utils.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <csignal>
#include <cerrno>

namespace tui {

class PosixTerminal : public ITerminal {
public:
    PosixTerminal() : cols_(80), rows_(24), raw_mode_(false), alt_screen_(false), resize_pending_(false) {
        g_active_terminal = this;
        detect_capabilities();
        update_size();
    }

    ~PosixTerminal() override {
        if (raw_mode_) leave_raw_mode();
        if (alt_screen_) leave_alternate_screen();
        cursor_show();
    }

    bool init() override {
        update_size();
        // Install SIGWINCH handler that sets a flag for the main loop to detect
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sigwinch_handler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGWINCH, &sa, nullptr);
        return true;
    }

    // Check if a resize event is pending and handle it
    // Returns true if the size changed
    bool check_resize(int& new_cols, int& new_rows) {
        if (!resize_pending_) return false;
        resize_pending_ = false;
        int old_cols = cols_;
        int old_rows = rows_;
        update_size();
        new_cols = cols_;
        new_rows = rows_;
        return (cols_ != old_cols || rows_ != old_rows);
    }

    void shutdown() override {
        leave_raw_mode();
        leave_alternate_screen();
        cursor_show();
        write("\033[0m"); // reset
        flush();
    }

    void clear() override {
        write("\033[2J\033[H");
    }

    void flush() override {
        fflush(stdout);
    }

    void sync() override {
        flush();
    }

    void cursor_hide() override { write("\033[?25l"); }
    void cursor_show() override { write("\033[?25h"); }

    void cursor_move(int x, int y) override {
        char buf[32];
        snprintf(buf, sizeof(buf), "\033[%d;%dH", y + 1, x + 1);
        write_raw(buf);
    }

    void cursor_save() override { write("\033[s"); }
    void cursor_restore() override { write("\033[u"); }

    int cols() const override { return cols_; }
    int rows() const override { return rows_; }

    void write(const std::string& text) override {
        fputs(text.c_str(), stdout);
    }

    void write_raw(const std::string& text) {
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
        const char* hline = has_cap(TerminalCaps::BoxDrawing) ? "\xe2\x94\x80" : "-"; // ─
        std::string line;
        for (int i = 0; i < w; i++) line += hline;
        write_styled_at(x, y, line, style);
    }

    void draw_vline(int x, int y, int h, const Style& style) override {
        const char* vline = has_cap(TerminalCaps::BoxDrawing) ? "\xe2\x94\x82" : "|"; // │
        for (int row = y; row < y + h; row++) {
            write_styled_at(x, row, vline, style);
        }
    }

    void draw_rect(int x, int y, int w, int h, const Style& style) override {
        if (w < 2 || h < 2) return;

        const char *tl, *tr, *bl, *br, *hline, *vline;
        if (has_cap(TerminalCaps::BoxDrawing)) {
            tl = "\xe2\x94\x8c"; // ┌
            tr = "\xe2\x94\x90"; // ┐
            bl = "\xe2\x94\x94"; // └
            br = "\xe2\x94\x98"; // ┘
            hline  = "\xe2\x94\x80"; // ─
            vline  = "\xe2\x94\x82"; // │
        } else {
            tl = "+"; tr = "+"; bl = "+"; br = "+"; hline = "-"; vline = "|";
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

    void draw_box(int x, int y, int w, int h, const Style& border_style,
                  const Style& fill_style) override {
        if (w < 2 || h < 2) return;
        draw_rect(x, y, w, h, border_style);
        if (w > 2 && h > 2) {
            for (int row = y + 1; row < y + h - 1; row++) {
                for (int col = x + 1; col < x + w - 1; col++) {
                    write_styled_at(col, row, " ", fill_style);
                }
            }
        }
    }

    void enter_raw_mode() override {
        if (raw_mode_) return;
        tcgetattr(STDIN_FILENO, &orig_termios_);
        struct termios raw = orig_termios_;
        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN] = 0;  // non-blocking
        raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        raw_mode_ = true;

        // Enable mouse (SGR 1006) and bracketed paste
        write("\033[?1006;1000;1015;2004h");
        flush();
    }

    void leave_raw_mode() override {
        if (!raw_mode_) return;
        // Disable mouse and bracketed paste
        write("\033[?1006;1000;1015;2004l");
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios_);
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
        write("\033]0;" + title + "\007");
    }

    TerminalCaps caps() const override { return caps_; }
    std::string term_type() const override {
        const char* term = getenv("TERM");
        return term ? term : "unknown";
    }

private:
    void update_size() {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            cols_ = ws.ws_col;
            rows_ = ws.ws_row;
        }
    }

    void detect_capabilities() {
        const char* term = getenv("TERM");
        if (!term) term = "xterm";

        caps_ = TerminalCaps::AnsiColors | TerminalCaps::Unicode |
                TerminalCaps::BoxDrawing | TerminalCaps::Resize |
                TerminalCaps::Mouse;

        // Most modern terminals support 256 colors and true color
        std::string t(term);
        if (t.find("256") != std::string::npos || t.find("xterm") != std::string::npos ||
            t.find("alacritty") != std::string::npos || t.find("kitty") != std::string::npos ||
            t.find("tmux") != std::string::npos) {
            caps_ = caps_ | TerminalCaps::Color256 | TerminalCaps::TrueColor;
        }
    }

    void emit_style(const Style& style) {
        char buf[64];
        int len = 0;
        buf[len++] = '\033';
        buf[len++] = '[';

        if (style.bold)      buf[len++] = '1', buf[len++] = ';';
        if (style.dim)       buf[len++] = '2', buf[len++] = ';';
        if (style.italic)    buf[len++] = '3', buf[len++] = ';';
        if (style.underline) buf[len++] = '4', buf[len++] = ';';
        if (style.blink)     buf[len++] = '5', buf[len++] = ';';
        if (style.reverse)   buf[len++] = '7', buf[len++] = ';';
        if (style.hidden)    buf[len++] = '8', buf[len++] = ';';

        // Foreground
        if (style.fg.mode == Color::Mode::TrueColor) {
            len += snprintf(buf + len, sizeof(buf) - len, "38;2;%d;%d;%d;",
                           style.fg.rgb.r, style.fg.rgb.g, style.fg.rgb.b);
        } else if (style.fg.mode == Color::Mode::Indexed) {
            len += snprintf(buf + len, sizeof(buf) - len, "38;5;%d;", style.fg.index);
        }

        // Background
        if (style.bg.mode == Color::Mode::TrueColor) {
            len += snprintf(buf + len, sizeof(buf) - len, "48;2;%d;%d;%d;",
                           style.bg.rgb.r, style.bg.rgb.g, style.bg.rgb.b);
        } else if (style.bg.mode == Color::Mode::Indexed) {
            len += snprintf(buf + len, sizeof(buf) - len, "48;5;%d;", style.bg.index);
        }

        if (len > 2) buf[len - 1] = 'm'; // replace last ';' with 'm'
        else buf[len++] = '0', buf[len++] = 'm';

        buf[len] = '\0';
        write_raw(std::string(buf, len));
    }

    int cols_, rows_;
    TerminalCaps caps_;
    bool raw_mode_;
    bool alt_screen_;
    volatile sig_atomic_t resize_pending_;
    struct termios orig_termios_;

    static void sigwinch_handler(int) {
        // We can't access instance members from a C signal handler.
        // Instead, we use a global pointer to the active terminal instance.
        // This is set when the terminal is created.
        if (g_active_terminal) {
            g_active_terminal->resize_pending_ = true;
        }
    }

    // Global pointer for signal handler access (set in constructor)
    static PosixTerminal* g_active_terminal;
};

// Static member initialization
PosixTerminal* PosixTerminal::g_active_terminal = nullptr;

ITerminal* create_terminal() {
    return new PosixTerminal();
}

void destroy_terminal(ITerminal* term) {
    delete term;
}

} // namespace tui
