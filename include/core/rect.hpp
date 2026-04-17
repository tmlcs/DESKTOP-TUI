#ifndef TUI_CORE_RECT_HPP
#define TUI_CORE_RECT_HPP

#include <algorithm>
#include <optional>

namespace tui {

struct Point {
    int x, y;

    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}

    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
    bool operator!=(const Point& o) const { return !(*this == o); }

    Point operator+(const Point& o) const { return {x + o.x, y + o.y}; }
    Point operator-(const Point& o) const { return {x - o.x, y - o.y}; }
};

struct Rect {
    int x, y, w, h;

    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
    Rect(Point pos, int w_, int h_) : x(pos.x), y(pos.y), w(w_), h(h_) {}

    int left()   const { return x; }
    int top()    const { return y; }
    int right()  const { return x + w; }
    int bottom() const { return y + h; }

    int center_x() const { return x + w / 2; }
    int center_y() const { return y + h / 2; }
    Point center() const { return {center_x(), center_y()}; }

    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    bool contains(Point p) const { return contains(p.x, p.y); }

    bool intersects(const Rect& o) const {
        return x < o.right() && right() > o.x &&
               y < o.bottom() && bottom() > o.y;
    }

    std::optional<Rect> intersection(const Rect& o) const {
        if (!intersects(o)) return std::nullopt;
        
        // Safe integer arithmetic with overflow protection
        int ix = std::max(x, o.x);
        int iy = std::max(y, o.y);
        int o_right = o.right();
        int o_bottom = o.bottom();
        int my_right = right();
        int my_bottom = bottom();
        
        int iw = std::min(my_right, o_right) - ix;
        int ih = std::min(my_bottom, o_bottom) - iy;
        
        // Validate dimensions to prevent negative/overflow results
        if (iw <= 0 || ih <= 0) return std::nullopt;
        
        return Rect{ix, iy, iw, ih};
    }

    Rect expand(int margin) const {
        return {x - margin, y - margin, w + 2 * margin, h + 2 * margin};
    }

    Rect shrink(int margin) const {
        return expand(-margin);
    }

    // Clamp to be within bounds
    Rect clamp(const Rect& bounds) const {
        int cx = std::max(bounds.x, x);
        int cy = std::max(bounds.y, y);
        int cw = std::min(right(), bounds.right()) - cx;
        int ch = std::min(bottom(), bounds.bottom()) - cy;
        if (cw <= 0 || ch <= 0) return {cx, cy, 0, 0};
        return {cx, cy, cw, ch};
    }

    bool operator==(const Rect& o) const {
        return x == o.x && y == o.y && w == o.w && h == o.h;
    }

    bool empty() const { return w <= 0 || h <= 0; }
};

} // namespace tui

#endif // TUI_CORE_RECT_HPP
