#ifndef TUI_CORE_COLORS_HPP
#define TUI_CORE_COLORS_HPP

#include <cstdint>
#include <string>

namespace tui {

/// 256-color palette index
using ColorIndex = uint8_t;

/// True color RGB
struct ColorRGB {
    uint8_t r, g, b;

    ColorRGB() : r(0), g(0), b(0) {}
    ColorRGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}

    // Common colors
    static ColorRGB Black()   { return {0, 0, 0}; }
    static ColorRGB White()   { return {255, 255, 255}; }
    static ColorRGB Red()     { return {255, 0, 0}; }
    static ColorRGB Green()   { return {0, 255, 0}; }
    static ColorRGB Blue()    { return {0, 0, 255}; }
    static ColorRGB Yellow()  { return {255, 255, 0}; }
    static ColorRGB Cyan()    { return {0, 255, 255}; }
    static ColorRGB Magenta() { return {255, 0, 255}; }
    static ColorRGB Gray()    { return {128, 128, 128}; }
    static ColorRGB DarkGray(){ return {64, 64, 64}; }

    bool operator==(const ColorRGB& o) const {
        return r == o.r && g == o.g && b == o.b;
    }
};

/// Color representation (index or true color)
struct Color {
    enum class Mode { Default, Indexed, TrueColor };

    Mode mode;
    union {
        ColorIndex index;
        ColorRGB rgb;
    };

    Color() : mode(Mode::Default) {}
    static Color DefaultColor() { Color c; c.mode = Mode::Default; return c; }
    static Color Pal(ColorIndex i) {
        Color c; c.mode = Mode::Indexed; c.index = i; return c;
    }
    static Color RGB(uint8_t r, uint8_t g, uint8_t b) {
        Color c; c.mode = Mode::TrueColor; c.rgb = {r, g, b}; return c;
    }
    static Color RGB(ColorRGB c) {
        Color col; col.mode = Mode::TrueColor; col.rgb = c; return col;
    }
};

/// Text styling attributes
/// 
/// @note THREAD SAFETY: This struct uses bitfields which share storage. While copying
///       a Style is safe, concurrent modification of the same Style instance from multiple
///       threads is NOT safe. The bitfield implementation means that writing to one field
///       (e.g., bold) could theoretically affect adjacent bitfields in a data race scenario.
///       
///       Always use value semantics (copy Styles) rather than shared mutable references
///       when working across thread boundaries. For single-threaded TUI usage, this is not a concern.
struct Style {
    Color fg;
    Color bg;
    bool bold      : 1;
    bool italic    : 1;
    bool underline : 1;
    bool blink     : 1;
    bool reverse   : 1;
    bool dim       : 1;
    bool hidden    : 1;

    Style() : fg(), bg(), bold(false), italic(false), underline(false),
              blink(false), reverse(false), dim(false), hidden(false) {}

    static Style Default() { return {}; }

    Style with_fg(Color c) const { Style s = *this; s.fg = c; return s; }
    Style with_bg(Color c) const { Style s = *this; s.bg = c; return s; }
    Style with_bold(bool b) const { Style s = *this; s.bold = b; return s; }
    Style with_reverse(bool b) const { Style s = *this; s.reverse = b; return s; }

    bool operator==(const Style& o) const {
        return fg.mode == o.fg.mode &&
               (fg.mode == Color::Mode::Default ||
                (fg.mode == Color::Mode::Indexed && fg.index == o.fg.index) ||
                (fg.mode == Color::Mode::TrueColor && fg.rgb == o.fg.rgb)) &&
               bg.mode == o.bg.mode &&
               (bg.mode == Color::Mode::Default ||
                (bg.mode == Color::Mode::Indexed && bg.index == o.bg.index) ||
                (bg.mode == Color::Mode::TrueColor && bg.rgb == o.bg.rgb)) &&
               bold == o.bold && italic == o.italic &&
               underline == o.underline && blink == o.blink &&
               reverse == o.reverse && dim == o.dim && hidden == o.hidden;
    }

    bool operator!=(const Style& o) const { return !(*this == o); }
};

// Compile-time size check to ensure Style remains compact
static_assert(sizeof(Style) <= 24, "Style should remain compact");

/// Predefined styles
namespace Styles {
    inline Style Normal()        { return Style::Default(); }
    inline Style Bold()          { Style s; s.bold = true; return s; }
    inline Style Title()         { Style s; s.bold = true; s.fg = Color::Pal(11); return s; }
    inline Style Header()        { Style s; s.bold = true; s.fg = Color::Pal(14); return s; }
    inline Style Success()       { Style s; s.fg = Color::Pal(10); return s; }
    inline Style Error()         { Style s; s.fg = Color::Pal(9); return s; }
    inline Style Warning()       { Style s; s.fg = Color::Pal(11); return s; }
    inline Style Info()          { Style s; s.fg = Color::Pal(12); return s; }
    inline Style Dim()           { Style s; s.dim = true; return s; }
    inline Style Selected()      { Style s; s.reverse = true; return s; }
    inline Style Border()        { Style s; s.fg = Color::Pal(8); return s; }
    inline Style BorderActive(){ Style s; s.fg = Color::Pal(14); s.bold = true; return s; }
}

/// DRY: Generate ANSI escape sequence for a Style (shared across all terminals)
/// @note PERFORMANCE: Uses pre-allocated buffer to avoid string allocations in hot path
inline std::string emit_style_to_string(const Style& style) {
    // Pre-allocate buffer: max ANSI sequence is ~60 bytes (true color fg+bg + attributes)
    char buffer[72];
    int pos = 0;
    
    buffer[pos++] = '\033';
    buffer[pos++] = '[';
    buffer[pos++] = '0'; // reset first

    if (style.bold)      { buffer[pos++] = ';'; buffer[pos++] = '1'; }
    if (style.dim)       { buffer[pos++] = ';'; buffer[pos++] = '2'; }
    if (style.italic)    { buffer[pos++] = ';'; buffer[pos++] = '3'; }
    if (style.underline) { buffer[pos++] = ';'; buffer[pos++] = '4'; }
    if (style.blink)     { buffer[pos++] = ';'; buffer[pos++] = '5'; }
    if (style.reverse)   { buffer[pos++] = ';'; buffer[pos++] = '7'; }
    if (style.hidden)    { buffer[pos++] = ';'; buffer[pos++] = '8'; }

    if (style.fg.mode == Color::Mode::TrueColor) {
        pos += std::snprintf(buffer + pos, sizeof(buffer) - pos, ";38;2;%u;%u;%u",
                            style.fg.rgb.r, style.fg.rgb.g, style.fg.rgb.b);
    } else if (style.fg.mode == Color::Mode::Indexed) {
        pos += std::snprintf(buffer + pos, sizeof(buffer) - pos, ";38;5;%u", style.fg.index);
    }

    if (style.bg.mode == Color::Mode::TrueColor) {
        pos += std::snprintf(buffer + pos, sizeof(buffer) - pos, ";48;2;%u;%u;%u",
                            style.bg.rgb.r, style.bg.rgb.g, style.bg.rgb.b);
    } else if (style.bg.mode == Color::Mode::Indexed) {
        pos += std::snprintf(buffer + pos, sizeof(buffer) - pos, ";48;5;%u", style.bg.index);
    }

    buffer[pos++] = 'm';
    buffer[pos] = '\0';
    
    return std::string(buffer, pos);
}

} // namespace tui

#endif // TUI_CORE_COLORS_HPP
