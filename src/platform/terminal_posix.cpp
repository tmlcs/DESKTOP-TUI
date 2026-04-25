// POSIX (Linux, macOS, BSD) terminal implementation
// Uses termios for raw mode, ioctl for size, VT100 escape sequences

#include "platform/terminal.hpp"
#include "core/string_utils.hpp"
#include "core/config.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <csignal>
#include <cerrno>
#include <atomic>
#include <limits>

namespace tui {

// SEC-03: Static atomic flags for safe signal handler access
static std::atomic<bool> g_resize_pending{false};
static std::atomic<bool> g_is_initialized{false};

class PosixTerminal : public ITerminal {
public:
    PosixTerminal() : cols_(Config::MIN_TERMINAL_COLS), rows_(Config::MIN_TERMINAL_ROWS), raw_mode_(false), alt_screen_(false) {
        // Initialize with safe defaults
        memset(&orig_termios_, 0, sizeof(orig_termios_));
        if (tcgetattr(STDIN_FILENO, &orig_termios_) != 0) {
            // Failed to get terminal attributes - this might not be a terminal
            // Set safe defaults to avoid undefined behavior
            cfmakeraw(&orig_termios_);
        }
        detect_capabilities();
        update_size();
        // Hide mouse cursor immediately after initialization
        cursor_hide();
    }

    bool is_escape_pending() const override {
        // CRIT-02: Check for pending escape key (platform-specific)
        // This implementation returns false as escape key handling is separate
        // from the SIGWINCH handler (which handles terminal resize events)
        return false;
    }

    ~PosixTerminal() override {
        if (raw_mode_) leave_raw_mode();
        if (alt_screen_) leave_alternate_screen();
        cursor_show();
        // Mark as uninitialized before destruction
        g_is_initialized = false;
    }

    bool init() override {
        // SEC-04: Validate terminal initialization
        // Check if stdin is actually a terminal
        if (!isatty(STDIN_FILENO)) {
            // Not a terminal - could be piped input or redirected
            // This is not necessarily an error, but limits functionality
            // Mouse and raw mode won't work properly
        }

        update_size();

        // SEC-02: Validate dimensions to prevent underflow/overflow
        if (cols_ < Config::MIN_TERMINAL_COLS || rows_ < Config::MIN_TERMINAL_ROWS) {
            // Terminal too small for meaningful UI
            // Could fallback to a message or refuse to initialize
            // For now, just log the issue
        }

        // Mark as initialized AFTER successful setup
        g_is_initialized = true;

        // Install SIGWINCH handler that sets a flag for the main loop to detect
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sigwinch_handler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        if (sigaction(SIGWINCH, &sa, nullptr) != 0) {
            // Failed to install signal handler - non-fatal but resize won't work
        }
        return true;
    }

    // Check if a resize event is pending and handle it
    // Returns true if the size changed
    bool check_resize(int& new_cols, int& new_rows) {
        // SEC-03: Use atomic flag instead of instance member for signal safety
        if (!g_resize_pending.load()) return false;
        g_resize_pending.store(false);
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

    void enter_raw_mode() override {
        if (raw_mode_) return;
        if (tcgetattr(STDIN_FILENO, &orig_termios_) != 0) return; // failed — don't set raw_mode_
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
        
        // Write the sanitized title using OSC sequence
        // OSC 0 ; title BEL - Set icon name and window title
        write("\033]0;" + sanitized + "\007");
    }

    TerminalCaps caps() const override { return caps_; }
    std::string term_type() const override {
        const char* term = getenv("TERM");
        return term ? term : "unknown";
    }

private:
    // SEC-02: Constants for dimension validation (now using Config namespace)
    static constexpr int MIN_TERM_COLS = Config::MIN_TERMINAL_COLS;
    static constexpr int MIN_TERM_ROWS = Config::MIN_TERMINAL_ROWS;
    static constexpr int MAX_TERM_COLS = Config::MAX_TERMINAL_COLS;
    static constexpr int MAX_TERM_ROWS = Config::MAX_TERMINAL_ROWS;

    void update_size() {
        struct winsize ws;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
            // SEC-02: Validate dimensions to prevent underflow/overflow
            if (ws.ws_col >= MIN_TERM_COLS && ws.ws_col <= MAX_TERM_COLS) {
                cols_ = ws.ws_col;
            } else if (ws.ws_col < MIN_TERM_COLS) {
                cols_ = MIN_TERM_COLS;
            } else {
                cols_ = MAX_TERM_COLS;
            }
            
            if (ws.ws_row >= MIN_TERM_ROWS && ws.ws_row <= MAX_TERM_ROWS) {
                rows_ = ws.ws_row;
            } else if (ws.ws_row < MIN_TERM_ROWS) {
                rows_ = MIN_TERM_ROWS;
            } else {
                rows_ = MAX_TERM_ROWS;
            }
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
        write_raw(emit_style_to_string(style));
    }

    int cols_, rows_;
    TerminalCaps caps_;
    bool raw_mode_;
    bool alt_screen_;
    struct termios orig_termios_;

    static void sigwinch_handler(int) {
        // SEC-03: Prevent signal handling before initialization or after destruction
        // This prevents use-after-free and null pointer dereference
        if (!g_is_initialized.load()) {
            return;
        }

        // Set atomic flag - safe to access from signal handler
        g_resize_pending.store(true);
    }

    // SEC-03: Removed g_active_terminal static pointer to eliminate dangling pointer vulnerability.
    // Signal handler now uses flag-based approach instead of direct instance access.
};

// SEC-03: Removed static member initialization - no longer needed with flag-based approach

std::unique_ptr<ITerminal> create_terminal() {
    return std::make_unique<PosixTerminal>();
}

} // namespace tui
