#ifndef TUI_UI_LABEL_HPP
#define TUI_UI_LABEL_HPP

#include "widget.hpp"
#include "ui/renderer.hpp"
#include "core/string_utils.hpp"
#include <string>
#include <vector>

namespace tui {

/// Label widget - displays text
class Label : public Widget {
public:
    enum Align { Left, Center, Right };

    Label() = default;
    Label(const std::string& text, const Style& style = Style::Default())
        : text_(text), style_(style) {}

    void set_text(const std::string& text) { text_ = text; }
    std::string text() const { return text_; }

    void set_style(const Style& s) { style_ = s; }
    void set_align(Align a) { align_ = a; }

    void render(Renderer& r) override {
        if (!visible_ || text_.empty()) return;

        std::vector<std::string> lines;
        if (bounds_.h > 1 && bounds_.w > 0) {
            lines = word_wrap(text_, bounds_.w);
        } else {
            lines.push_back(text_);
        }

        int y = bounds_.y;
        for (size_t i = 0; i < lines.size() && y < bounds_.y + bounds_.h; i++, y++) {
            const auto& line = lines[i];
            switch (align_) {
                case Left:
                    r.write(bounds_.x, y, line, style_);
                    break;
                case Center:
                    r.write_center(bounds_.x, y, line, bounds_.w, style_);
                    break;
                case Right:
                    r.write_right(bounds_.x, y, line, bounds_.w, style_);
                    break;
            }
        }
    }

private:
    std::string text_;
    Style style_;
    Align align_ = Left;
};

} // namespace tui

#endif // TUI_UI_LABEL_HPP
