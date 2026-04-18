#ifndef TUI_UI_WIDGET_HPP
#define TUI_UI_WIDGET_HPP

#include "core/common.hpp"
#include <string>
#include <vector>
#include <memory>

namespace tui {

class Renderer;

/// Base widget class
class Widget {
public:
    Widget() = default;
    virtual ~Widget() = default;

    // Position and size
    virtual void set_bounds(const Rect& r) { bounds_ = r; }
    virtual Rect bounds() const { return bounds_; }
    virtual int x() const { return bounds_.x; }
    virtual int y() const { return bounds_.y; }
    virtual int width() const { return bounds_.w; }
    virtual int height() const { return bounds_.h; }

    // Visibility
    virtual void show() { visible_ = true; }
    virtual void hide() { visible_ = false; }
    virtual bool visible() const { return visible_; }

    // Focus
    virtual void focus() { focused_ = true; on_focus(); }
    virtual void blur() { focused_ = false; on_blur(); }
    virtual bool focused() const { return focused_; }
    virtual bool can_focus() const { return focusable_; }
    virtual void set_focusable(bool f) { focusable_ = f; }

    // Rendering
    virtual void render(Renderer& r) = 0;

    // Event handling
    virtual bool handle_event(const Event& e) {
        (void)e;
        return false; // not handled
    }

    // Resize handling
    virtual void on_resize(int cols, int rows) { (void)cols; (void)rows; }

    // Signals
    void on_click(std::function<void()> cb) { on_click_ = std::move(cb); }

protected:
    virtual void on_focus() {}
    virtual void on_blur() {}

    void emit_click() { if (on_click_) on_click_(); }

    Rect bounds_;
    bool visible_ = true;
    bool focused_ = false;
    bool focusable_ = false;
    std::function<void()> on_click_;
};

} // namespace tui

#endif // TUI_UI_WIDGET_HPP
