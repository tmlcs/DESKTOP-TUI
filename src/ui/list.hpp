#ifndef TUI_UI_LIST_HPP
#define TUI_UI_LIST_HPP

#include "widget.hpp"
#include "ui/renderer.hpp"
#include <string>
#include <vector>
#include <functional>

namespace tui {

/// List widget - scrollable list of items
class List : public Widget {
public:
    List() { focusable_ = true; }

    void add_item(const std::string& text) { items_.push_back(text); }
    void set_items(std::vector<std::string> items) { items_ = std::move(items); }
    const std::vector<std::string>& items() const { return items_; }

    int selected() const { return selected_; }
    void set_selected(int idx) {
        if (idx >= 0 && idx < static_cast<int>(items_.size())) {
            selected_ = idx;
            ensure_visible();
        }
    }

    void on_select(std::function<void(int)> cb) { on_select_ = std::move(cb); }

    void render(Renderer& r) override {
        if (!visible_) return;

        for (int i = 0; i < bounds_.h && scroll_ + i < static_cast<int>(items_.size()); i++) {
            int item_idx = scroll_ + i;
            bool is_selected = item_idx == selected_;
            bool is_focused = focused_;

            Style item_style = Style::Default();
            if (is_selected && is_focused) {
                item_style = Styles::Selected();
            } else if (is_selected) {
                item_style = Styles::Dim();
            }

            std::string display = items_[item_idx];
            if (static_cast<int>(display.size()) > bounds_.w) {
                display = display.substr(0, bounds_.w);
            }

            r.write(bounds_.x, bounds_.y + i, display, item_style);
        }
    }

    bool handle_event(const Event& e) override {
        if (!visible_ || !focused_) return false;

        switch (e.type) {
            case EventType::KeyPress:
                switch (e.key_code) {
                    case Keys::ArrowUp:
                        if (selected_ > 0) {
                            set_selected(selected_ - 1);
                            if (on_select_) on_select_(selected_);
                        }
                        return true;
                    case Keys::ArrowDown:
                        if (selected_ < static_cast<int>(items_.size()) - 1) {
                            set_selected(selected_ + 1);
                            if (on_select_) on_select_(selected_);
                        }
                        return true;
                    case Keys::PageUp:
                        set_selected(selected_ - bounds_.h);
                        if (on_select_) on_select_(selected_);
                        return true;
                    case Keys::PageDown:
                        set_selected(selected_ + bounds_.h);
                        if (on_select_) on_select_(selected_);
                        return true;
                    case Keys::Home:
                        set_selected(0);
                        if (on_select_) on_select_(selected_);
                        return true;
                    case Keys::End:
                        set_selected(static_cast<int>(items_.size()) - 1);
                        if (on_select_) on_select_(selected_);
                        return true;
                    case Keys::Enter:
                        emit_click();
                        return true;
                }
                break;
            default:
                break;
        }
        return false;
    }

    void clear() { items_.clear(); selected_ = 0; scroll_ = 0; }
    int size() const { return static_cast<int>(items_.size()); }

private:
    void ensure_visible() {
        if (selected_ < scroll_) {
            scroll_ = selected_;
        } else if (selected_ >= scroll_ + bounds_.h) {
            scroll_ = selected_ - bounds_.h + 1;
        }
    }

    std::vector<std::string> items_;
    int selected_ = 0;
    int scroll_ = 0;
    std::function<void(int)> on_select_;
};

} // namespace tui

#endif // TUI_UI_LIST_HPP
