#ifndef TUI_UI_PANEL_HPP
#define TUI_UI_PANEL_HPP

#include "widget.hpp"
#include "ui/renderer.hpp"
#include <vector>
#include <memory>
#include <string>

namespace tui {

/// Panel widget - a container with optional border and title
class Panel : public Widget {
public:
    Panel() { focusable_ = false; }

    explicit Panel(const std::string& title, const Style& border_style = Styles::Border())
        : title_(title), border_style_(border_style) {
        focusable_ = false;
    }

    void set_title(const std::string& title) { title_ = title; }
    std::string title() const { return title_; }

    void set_border_style(const Style& s) { border_style_ = s; }
    void set_show_border(bool show) { show_border_ = show; }

    // Child management
    void add_child(std::shared_ptr<Widget> child) {
        children_.push_back(std::move(child));
    }

    void remove_child(Widget* child) {
        children_.erase(
            std::remove_if(children_.begin(), children_.end(),
                [child](const auto& c) { return c.get() == child; }),
            children_.end()
        );
    }

    void clear_children() { children_.clear(); }

    const std::vector<std::shared_ptr<Widget>>& children() const { return children_; }

    // Find focused child
    Widget* focused_child() {
        for (auto& c : children_) {
            if (c->focused()) return c.get();
        }
        return nullptr;
    }

    void render(Renderer& r) override {
        if (!visible_) return;

        if (show_border_) {
            r.draw_box(bounds_, border_style_, fill_style_);
            // Title
            if (!title_.empty() && bounds_.w > 4) {
                std::string title_display = " " + title_ + " ";
                r.write(bounds_.x + (bounds_.w - static_cast<int>(title_display.size())) / 2,
                        bounds_.y, title_display, Styles::Header());
            }
        }

        // Render children
        Rect inner = content_area();
        for (auto& child : children_) {
            if (child->visible()) {
                child->render(r);
            }
        }
    }

    bool handle_event(const Event& e) override {
        // For mouse events, first do hit-testing against children
        if (e.type == EventType::MouseDown || e.type == EventType::MouseScroll) {
            // Test children in reverse z-order (topmost first)
            for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
                auto& child = *it;
                if (child->visible() && child->can_focus()) {
                    Rect cb = child->bounds();
                    if (cb.contains(e.mouse_x, e.mouse_y)) {
                        // Give focus to this child
                        if (!child->focused()) {
                            if (focused_child()) focused_child()->blur();
                            child->focus();
                        }
                        return child->handle_event(e);
                    }
                }
            }
            // Click didn't hit any focusable child, blur focused child
            if (focused_child()) focused_child()->blur();
        }

        // Try focused child first
        for (auto& child : children_) {
            if (child->focused() && child->visible() && child->can_focus()) {
                if (child->handle_event(e)) return true;
            }
        }
        // Then try all visible children
        for (auto& child : children_) {
            if (child->visible() && child.get() != focused_child()) {
                if (child->handle_event(e)) return true;
            }
        }
        return false;
    }

    // Get the inner content area (excluding border)
    Rect content_area() const {
        if (!show_border_) return bounds_;
        return {bounds_.x + 1, bounds_.y + 1, bounds_.w - 2, bounds_.h - 2};
    }

    void on_resize(int cols, int rows) override {
        for (auto& child : children_) {
            child->on_resize(cols, rows);
        }
    }

protected:
    std::string title_;
    Style border_style_;
    Style fill_style_;
    bool show_border_ = true;
    std::vector<std::shared_ptr<Widget>> children_;
};

} // namespace tui

#endif // TUI_UI_PANEL_HPP
