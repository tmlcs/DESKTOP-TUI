#ifndef TUI_UI_BORDER_HPP
#define TUI_UI_BORDER_HPP

#include "widget.hpp"
#include "ui/renderer.hpp"

namespace tui {

/// Border widget - draws a bordered rectangle
class Border : public Widget {
public:
    Border() = default;

    explicit Border(const Style& style = Styles::Border())
        : border_style_(style) {}

    void set_style(const Style& s) { border_style_ = s; }

    void render(Renderer& r) override {
        if (!visible_) return;
        r.draw_border(bounds_, border_style_);
    }

private:
    Style border_style_ = Styles::Border();
};

} // namespace tui

#endif // TUI_UI_BORDER_HPP
