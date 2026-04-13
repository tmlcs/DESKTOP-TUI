#ifndef TUI_WINDOW_WINDOW_HPP
#define TUI_WINDOW_WINDOW_HPP

#include "core/rect.hpp"
#include "core/event.hpp"
#include "core/colors.hpp"
#include "ui/widget.hpp"
#include "ui/panel.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <functional>

namespace tui {

using WindowId = uint64_t;

/// A window in the window system
class Window {
public:
    Window() : id_(next_id_++) {}

    explicit Window(const std::string& title, Rect bounds = {})
        : id_(next_id_++), title_(title), bounds_(bounds) {
        if (bounds_.w == 0) bounds_.w = 40;
        if (bounds_.h == 0) bounds_.h = 12;
    }

    WindowId id() const { return id_; }

    // Properties
    void set_title(const std::string& title) { title_ = title; }
    std::string title() const { return title_; }

    void set_bounds(Rect r) { bounds_ = r; on_move(); }
    Rect bounds() const { return bounds_; }

    void move(int x, int y) { bounds_.x = x; bounds_.y = y; on_move(); }
    void resize(int w, int h) { bounds_.w = w; bounds_.h = h; on_resize(); }

    // Visibility and state
    void show() { visible_ = true; }
    void hide() { visible_ = false; }
    bool visible() const { return visible_ && !minimized_; }
    bool is_minimized() const { return minimized_; }

    void minimize() { minimized_ = true; }
    void restore() { minimized_ = false; }
    void toggle_minimize() { minimized_ = !minimized_; }

    // Focus
    void focus() { focused_ = true; }
    void blur() { focused_ = false; }
    bool is_focused() const { return focused_; }

    // Content - the window contains a root panel
    void set_content(std::shared_ptr<Panel> panel) { content_ = std::move(panel); }
    std::shared_ptr<Panel> content() const { return content_; }

    // Render the window
    void render(class Renderer& r) {
        if (!visible_ || minimized_) return;

        if (content_) {
            content_->set_bounds(bounds_);
            content_->render(r);
        } else {
            // Default: draw window frame
            Style border = focused_ ? Styles::BorderActive() : Styles::Border();
            r.draw_box(bounds_, border, Style::Default());

            // Title
            if (bounds_.w > 4) {
                std::string title = " " + title_ + " ";
                int title_x = bounds_.x + (bounds_.w - static_cast<int>(title.size())) / 2;
                r.write(title_x, bounds_.y, title, Styles::Title());
            }
        }
    }

    // Event handling
    bool handle_event(const Event& e) {
        if (!visible_ || minimized_) return false;
        if (content_) {
            return content_->handle_event(e);
        }
        return false;
    }

    // Resize notification
    void on_resize(int cols, int rows) {
        (void)cols; (void)rows;
        if (content_) content_->on_resize(cols, rows);
    }

    // Z-order
    int z_order() const { return z_order_; }
    void set_z_order(int z) { z_order_ = z; }

    // Callbacks
    std::function<void()> on_moved;
    std::function<void()> on_resized;

private:
    void on_move() { if (content_) content_->set_bounds(bounds_); }
    void on_resize() { if (content_) content_->set_bounds(bounds_); }

    WindowId id_;
    std::string title_;
    Rect bounds_;
    bool visible_ = true;
    bool focused_ = false;
    bool minimized_ = false;
    int z_order_ = 0;
    std::shared_ptr<Panel> content_;

    static WindowId next_id_;
};

} // namespace tui

#endif // TUI_WINDOW_WINDOW_HPP
