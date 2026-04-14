#ifndef TUI_PLATFORM_TERMINAL_HPP
#define TUI_PLATFORM_TERMINAL_HPP

#include "core/colors.hpp"
#include "core/rect.hpp"
#include <string>
#include <cstdint>

namespace tui {

/// Terminal capability flags
enum class TerminalCaps : uint8_t {
    None        = 0,
    AnsiColors  = 1 << 0,  // Basic ANSI 16 colors
    Color256    = 1 << 1,  // 256 color palette
    TrueColor   = 1 << 2,  // 24-bit RGB
    Mouse       = 1 << 3,  // Mouse events (SGR 1006)
    Unicode     = 1 << 4,  // Unicode/UTF-8 support
    BoxDrawing  = 1 << 5,  // Box drawing characters
    Resize      = 1 << 6,  // SIGWINCH resize events
};

inline TerminalCaps operator|(TerminalCaps a, TerminalCaps b) {
    return static_cast<TerminalCaps>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline TerminalCaps operator&(TerminalCaps a, TerminalCaps b) {
    return static_cast<TerminalCaps>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

/// Abstract terminal interface
class ITerminal {
public:
    virtual ~ITerminal() = default;

    // Lifecycle
    virtual bool init() = 0;
    virtual void shutdown() = 0;

    // Screen control
    virtual void clear() = 0;
    virtual void flush() = 0;
    virtual void sync() = 0; // flush + wait for output

    // Cursor
    virtual void cursor_hide() = 0;
    virtual void cursor_show() = 0;
    virtual void cursor_move(int x, int y) = 0;
    virtual void cursor_save() = 0;
    virtual void cursor_restore() = 0;

    // Dimensions
    virtual int cols() const = 0;
    virtual int rows() const = 0;
    virtual Rect bounds() const { return {0, 0, cols(), rows()}; }

    // Writing
    virtual void write(const std::string& text) = 0;
    virtual void write_at(int x, int y, const std::string& text) = 0;
    virtual void write_styled(const std::string& text, const Style& style) = 0;
    virtual void write_styled_at(int x, int y, const std::string& text, const Style& style) = 0;

    // Fill region
    virtual void fill(int x, int y, int w, int h, char ch, const Style& style) = 0;
    virtual void clear_region(int x, int y, int w, int h) = 0;

    // Line drawing (uses box-drawing chars or ASCII fallback)
    virtual void draw_hline(int x, int y, int w, const Style& style) = 0;
    virtual void draw_vline(int x, int y, int h, const Style& style) = 0;
    virtual void draw_rect(int x, int y, int w, int h, const Style& style) = 0;
    virtual void draw_box(int x, int y, int w, int h, const Style& border_style,
                          const Style& fill_style) {
        // DRY: default implementation using draw_rect + write_styled_at
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

    // Mode control
    virtual void enter_raw_mode() = 0;
    virtual void leave_raw_mode() = 0;
    virtual void enter_alternate_screen() = 0;
    virtual void leave_alternate_screen() = 0;

    // Title
    virtual void set_title(const std::string& title) = 0;

    // Capabilities
    virtual TerminalCaps caps() const = 0;
    virtual bool has_cap(TerminalCaps cap) const { return (caps() & cap) != TerminalCaps::None; }

    // Platform-specific: detect terminal type
    virtual std::string term_type() const = 0;

    // Check for pending resize (platform-specific)
    // Default implementation returns false (no resize detection)
    virtual bool check_resize(int& new_cols, int& new_rows) {
        (void)new_cols; (void)new_rows;
        return false;
    }
};

/// Factory: create the appropriate terminal for this platform
ITerminal* create_terminal();
void destroy_terminal(ITerminal* term);

} // namespace tui

#endif // TUI_PLATFORM_TERMINAL_HPP
