#ifndef TUI_UI_RENDERER_HPP
#define TUI_UI_RENDERER_HPP

#include "platform/terminal.hpp"
#include "core/colors.hpp"
#include "core/rect.hpp"
#include "core/string_utils.hpp"
#include <vector>
#include <string>
#include <memory>

namespace tui {

/// A single cell in the render buffer
struct Cell {
    char32_t ch;
    Style style;

    Cell() : ch(' '), style() {}
    Cell(char32_t c, const Style& s) : ch(c), style(s) {}

    bool operator==(const Cell& o) const { return ch == o.ch && style == o.style; }
    bool operator!=(const Cell& o) const { return !(*this == o); }
};

/// Double-buffered renderer with dirty-region optimization
class Renderer {
public:
    Renderer(ITerminal& term) : term_(term) {}

    // Initialize renderer with terminal dimensions
    void init() {
        resize(term_.cols(), term_.rows());
    }

    // Resize buffers
    void resize(int cols, int rows) {
        bool size_changed = (cols_ != cols || rows_ != rows);
        cols_ = cols;
        rows_ = rows;
        back_buffer_.assign(cols * rows, Cell());
        if (size_changed) {
            // Reset front buffer to force full redraw on new dimensions
            front_buffer_.assign(cols * rows, Cell());
        }
        dirty_ = true;
    }

    // Clear the back buffer
    void clear() {
        for (auto& cell : back_buffer_) {
            cell = Cell(' ', Style::Default());
        }
        dirty_ = true;
    }

    // Clear a region in the back buffer
    void clear_region(int x, int y, int w, int h) {
        int x0 = std::max(0, x);
        int y0 = std::max(0, y);
        int x1 = std::min(cols_, x + w);
        int y1 = std::min(rows_, y + h);
        for (int row = y0; row < y1; row++) {
            for (int col = x0; col < x1; col++) {
                back_buffer_[row * cols_ + col] = Cell(' ', Style::Default());
            }
        }
        dirty_ = true;
    }

    // Write text at position (clipped to bounds)
    void write(int x, int y, const std::string& text) {
        write(x, y, text, Style::Default());
    }

    // Write styled text at position
    void write(int x, int y, const std::string& text, const Style& style) {
        if (y >= rows_ || y < -1000) return; // completely off-screen
        int col = x;
        int row = y;

        // FIX C1: decode UTF-8 by codepoint, not byte-by-byte
        const char* p = text.data();
        const char* end = p + text.size();
        while (p < end) {
            char32_t ch = utf8_decode(p, end);
            if (ch == 0 && p >= end) break;

            if (ch == '\n') {
                col = x;
                row++;
                continue;
            }
            if (col >= 0 && row >= 0 && col < cols_ && row < rows_) {
                Cell& cell = back_buffer_[row * cols_ + col];
                cell.ch = ch;
                cell.style = style;
                dirty_ = true;
            }
            col++;
        }
    }

    // Write a single character
    void put(int x, int y, char32_t ch, const Style& style) {
        if (x >= 0 && y >= 0 && x < cols_ && y < rows_) {
            Cell& cell = back_buffer_[y * cols_ + x];
            cell.ch = ch;
            cell.style = style;
            dirty_ = true;
        }
    }

    // Fill a rectangle in the back buffer
    void fill_rect(int x, int y, int w, int h, char32_t ch, const Style& style) {
        int x0 = std::max(0, x);
        int y0 = std::max(0, y);
        int x1 = std::min(cols_, x + w);
        int y1 = std::min(rows_, y + h);
        for (int row = y0; row < y1; row++) {
            for (int col = x0; col < x1; col++) {
                back_buffer_[row * cols_ + col] = Cell(ch, style);
            }
        }
        dirty_ = true;
    }

    // Draw a box (border + fill)
    void draw_box(const Rect& rect, const Style& border, const Style& fill) {
        if (rect.w < 2 || rect.h < 2) return;

        // Border characters
        bool unicode = term_.has_cap(TerminalCaps::BoxDrawing);
        const char32_t tl = unicode ? 0x250C : '+'; // ┌
        const char32_t tr = unicode ? 0x2510 : '+'; // ┐
        const char32_t bl = unicode ? 0x2514 : '+'; // └
        const char32_t br = unicode ? 0x2518 : '+'; // ┘
        const char32_t h  = unicode ? 0x2500 : '-'; // ─
        const char32_t v  = unicode ? 0x2502 : '|'; // │

        put(rect.x, rect.y, tl, border);
        put(rect.x + rect.w - 1, rect.y, tr, border);
        put(rect.x, rect.y + rect.h - 1, bl, border);
        put(rect.x + rect.w - 1, rect.y + rect.h - 1, br, border);

        for (int i = 1; i < rect.w - 1; i++) {
            put(rect.x + i, rect.y, h, border);
            put(rect.x + i, rect.y + rect.h - 1, h, border);
        }
        for (int row = 1; row < rect.h - 1; row++) {
            put(rect.x, rect.y + row, v, border);
            put(rect.x + rect.w - 1, rect.y + row, v, border);
        }

        // Fill
        fill_rect(rect.x + 1, rect.y + 1, rect.w - 2, rect.h - 2, ' ', fill);
    }

    // Draw border only
    void draw_border(const Rect& rect, const Style& border) {
        if (rect.w < 2 || rect.h < 2) return;
        bool unicode = term_.has_cap(TerminalCaps::BoxDrawing);
        const char32_t tl = unicode ? 0x250C : '+';
        const char32_t tr = unicode ? 0x2510 : '+';
        const char32_t bl = unicode ? 0x2514 : '+';
        const char32_t br = unicode ? 0x2518 : '+';
        const char32_t h  = unicode ? 0x2500 : '-';
        const char32_t v  = unicode ? 0x2502 : '|';

        put(rect.x, rect.y, tl, border);
        put(rect.x + rect.w - 1, rect.y, tr, border);
        put(rect.x, rect.y + rect.h - 1, bl, border);
        put(rect.x + rect.w - 1, rect.y + rect.h - 1, br, border);
        for (int i = 1; i < rect.w - 1; i++) {
            put(rect.x + i, rect.y, h, border);
            put(rect.x + i, rect.y + rect.h - 1, h, border);
        }
        for (int row = 1; row < rect.h - 1; row++) {
            put(rect.x, rect.y + row, v, border);
            put(rect.x + rect.w - 1, rect.y + row, v, border);
        }
    }

    // Center text in a region (relative to base_x)
    void write_center(int base_x, int y, const std::string& text, int width, const Style& style) {
        int display_len = static_cast<int>(text.size());
        if (display_len >= width) {
            write(base_x, y, text.substr(0, static_cast<size_t>(std::max(0, width))), style);
            return;
        }
        int start_x = base_x + (width - display_len) / 2;
        write(start_x, y, text, style);
    }

    // Right-align text within a region
    void write_right(int x, int y, const std::string& text, int max_width, const Style& style) {
        int display_len = static_cast<int>(text.size());
        int available = std::max(0, max_width);
        if (display_len >= available) {
            // Text too wide, truncate
            write(x, y, text.substr(0, static_cast<size_t>(available)), style);
            return;
        }
        int start_x = x + max_width - display_len;
        write(start_x, y, text, style);
    }

    // Flush dirty regions to terminal
    void flush() {
        if (!dirty_) return;

        // Full screen redraw (simplified; a more advanced version would track
        // per-row dirty rects and use cursor positioning optimization)
        term_.cursor_hide();

        Style current_style = Style::Default();
        bool style_set = false;

        for (int row = 0; row < rows_; row++) {
            bool row_changed = false;
            for (int col = 0; col < cols_; col++) {
                const Cell& back = back_buffer_[row * cols_ + col];
                const Cell& front = front_buffer_[row * cols_ + col];
                if (back != front) {
                    row_changed = true;
                    break;
                }
            }
            if (!row_changed) continue;

            // Move cursor to start of row
            term_.cursor_move(0, row);

            // Write entire row with style runs
            int col = 0;
            while (col < cols_) {
                const Cell& cell = back_buffer_[row * cols_ + col];

                // Find run of same style
                int run_end = col + 1;
                while (run_end < cols_ &&
                       back_buffer_[row * cols_ + run_end].style == cell.style) {
                    run_end++;
                }

                // Emit style change
                if (!style_set || !(cell.style == current_style)) {
                    emit_style(cell.style);
                    current_style = cell.style;
                    style_set = true;
                }

                // Write the run
                std::string run;
                for (int i = col; i < run_end; i++) {
                    run += utf8_encode(back_buffer_[row * cols_ + i].ch);
                }
                term_.write(run);

                // Update front buffer
                for (int i = col; i < run_end; i++) {
                    front_buffer_[row * cols_ + i] = back_buffer_[row * cols_ + i];
                }

                col = run_end;
            }
        }

        term_.write("\033[0m");
        term_.cursor_show();
        term_.flush();

        dirty_ = false;
    }

    // Mark entire screen dirty
    void mark_dirty() { dirty_ = true; }

    // Mark specific region dirty (optimized: only those rows need redraw)
    void mark_dirty(int /*x*/, int y, int /*w*/, int h) {
        // Clamp to bounds and mark the rows
        int y0 = std::max(0, y);
        int y1 = std::min(rows_, y + h);
        for (int row = y0; row < y1; row++) {
            // Force row dirty by zeroing front buffer for that row
            for (int col = 0; col < cols_; col++) {
                front_buffer_[row * cols_ + col] = Cell(); // different from back => dirty
            }
        }
        dirty_ = true;
    }

    int cols() const { return cols_; }
    int rows() const { return rows_; }
    Rect bounds() const { return {0, 0, cols_, rows_}; }

private:
    int bounds_clamp_x(int x) const { return std::max(0, std::min(x, cols_ - 1)); }
    void emit_style(const Style& style) {
        term_.write(emit_style_to_string(style));
    }

    std::string utf8_encode(char32_t ch) {
        if (ch < 0x80) return std::string(1, static_cast<char>(ch));
        if (ch < 0x800) {
            return std::string({
                static_cast<char>(0xC0 | (ch >> 6)),
                static_cast<char>(0x80 | (ch & 0x3F))
            });
        }
        if (ch < 0x10000) {
            return std::string({
                static_cast<char>(0xE0 | (ch >> 12)),
                static_cast<char>(0x80 | ((ch >> 6) & 0x3F)),
                static_cast<char>(0x80 | (ch & 0x3F))
            });
        }
        return std::string({
            static_cast<char>(0xF0 | (ch >> 18)),
            static_cast<char>(0x80 | ((ch >> 12) & 0x3F)),
            static_cast<char>(0x80 | ((ch >> 6) & 0x3F)),
            static_cast<char>(0x80 | (ch & 0x3F))
        });
    }

    ITerminal& term_;
    std::vector<Cell> back_buffer_;
    std::vector<Cell> front_buffer_;
    int cols_ = 0;
    int rows_ = 0;
    bool dirty_ = true;
};

} // namespace tui

#endif // TUI_UI_RENDERER_HPP
