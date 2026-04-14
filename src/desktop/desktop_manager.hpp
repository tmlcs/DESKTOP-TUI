#ifndef TUI_DESKTOP_DESKTOP_MANAGER_HPP
#define TUI_DESKTOP_DESKTOP_MANAGER_HPP

#include "desktop.hpp"
#include "ui/renderer.hpp"
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <functional>

namespace tui {

/// Manages multiple virtual desktops
class DesktopManager {
public:
    using DesktopPtr = std::shared_ptr<Desktop>;

    DesktopManager() {
        // Create initial desktop
        add_desktop("Desktop 1");
    }

    explicit DesktopManager(int initial_count) {
        for (int i = 0; i < initial_count; i++) {
            add_desktop("Desktop " + std::to_string(i + 1));
        }
    }

    // Add a new desktop
    DesktopPtr add_desktop(const std::string& name = "") {
        std::string desktop_name = name.empty() ?
            "Desktop " + std::to_string(desktops_.size() + 1) : name;
        auto desktop = std::make_shared<Desktop>(desktop_name);
        desktops_.push_back(desktop);
        on_desktop_added_(*desktop);
        return desktop;
    }

    // Remove a desktop (moves windows to active desktop)
    void remove_desktop(DesktopId id) {
        if (desktops_.size() <= 1) return; // Keep at least one

        auto it = std::find_if(desktops_.begin(), desktops_.end(),
            [id](const auto& d) { return d->id() == id; });
        if (it != desktops_.end()) {
            auto* active = active_desktop();
            // Move windows to active desktop
            for (auto& win : (*it)->windows()) {
                active->add_window(win);
            }
            desktops_.erase(it);
            // Fix active index if needed
            if (active_index_ >= static_cast<int>(desktops_.size())) {
                active_index_ = static_cast<int>(desktops_.size()) - 1;
            }
        }
    }

    // Switch to a desktop by index
    bool switch_to(int index) {
        if (index < 0 || index >= static_cast<int>(desktops_.size())) return false;
        if (active_) active_->set_active(false);
        active_index_ = index;
        active_ = desktops_[index].get();
        active_->set_active(true);
        on_switched_(*active_);
        return true;
    }

    // Switch to next desktop
    void switch_next() {
        if (desktops_.empty()) return;
        int next = (active_index_ + 1) % static_cast<int>(desktops_.size());
        switch_to(next);
    }

    // Switch to previous desktop
    void switch_prev() {
        if (desktops_.empty()) return;
        int prev = (active_index_ - 1 + static_cast<int>(desktops_.size())) % static_cast<int>(desktops_.size());
        switch_to(prev);
    }

    // Get active desktop
    Desktop* active_desktop() { return active_; }
    const Desktop* active_desktop() const { return active_; }

    int active_index() const { return active_index_; }
    int desktop_count() const { return static_cast<int>(desktops_.size()); }

    // Get desktop by index
    Desktop* get_desktop(int index) {
        if (index < 0 || index >= static_cast<int>(desktops_.size())) return nullptr;
        return desktops_[index].get();
    }

    // Get desktop by ID
    Desktop* find_desktop(DesktopId id) {
        for (auto& d : desktops_) {
            if (d->id() == id) return d.get();
        }
        return nullptr;
    }

    // All desktops
    const std::vector<DesktopPtr>& all_desktops() const { return desktops_; }

    // Move a window to a specific desktop
    void move_window_to_desktop(WindowId win_id, int desktop_index) {
        if (desktop_index < 0 || desktop_index >= static_cast<int>(desktops_.size())) return;
        // Find and remove from current desktop, add to target
        for (auto& d : desktops_) {
            auto* win = d->find_window(win_id);
            if (win) {
                // Get the shared_ptr from the source before removing
                for (auto& w : d->windows()) {
                    if (w->id() == win_id) {
                        d->remove_window(win_id);
                        desktops_[desktop_index]->add_window(w);
                        return;
                    }
                }
            }
        }
    }

    // Render the desktop indicator bar (workspace dots at bottom)
    void render_indicator(Renderer& r, int y) {
        int total_width = r.cols();
        int dot_spacing = 2;
        int total_dots_width = static_cast<int>(desktops_.size()) * (1 + dot_spacing);
        int start_x = (total_width - total_dots_width) / 2;

        for (size_t i = 0; i < desktops_.size(); i++) {
            bool is_active = static_cast<int>(i) == active_index_;
            bool has_windows = desktops_[i]->window_count() > 0;

            Style style = Styles::Dim();
            if (is_active) {
                style = Styles::Title();
            } else if (has_windows) {
                style = Styles::Normal();
            }

            const char* dot = is_active ? "●" : (has_windows ? "○" : "·");
            r.write(start_x + static_cast<int>(i) * (1 + dot_spacing), y, dot, style);
        }
    }

    // Render all windows on the active desktop
    void render_active_desktop(Renderer& r) {
        if (active_) {
            for (auto& win : active_->windows()) {
                if (win->visible()) {
                    win->render(r);
                }
            }
        }
    }

    // Dispatch event to active desktop's focused window
    bool dispatch_event(const Event& e) {
        if (!active_) return false;
        // Find focused window on active desktop
        for (auto& win : active_->windows()) {
            if (win->is_focused() && win->visible()) {
                return win->handle_event(e);
            }
        }
        return false;
    }

    // Resize
    void on_resize(int cols, int rows) {
        for (auto& d : desktops_) {
            d->on_resize(cols, rows);
            for (auto& win : d->windows()) {
                win->on_resize(cols, rows);
            }
        }
    }

    // Callbacks
    void on_switched(std::function<void(Desktop&)> cb) { on_switched_ = std::move(cb); }
    void on_desktop_added(std::function<void(Desktop&)> cb) { on_desktop_added_ = std::move(cb); }

private:
    std::vector<DesktopPtr> desktops_;
    int active_index_ = 0;
    Desktop* active_ = nullptr;

    std::function<void(Desktop&)> on_switched_ = [](Desktop&){};
    std::function<void(Desktop&)> on_desktop_added_ = [](Desktop&){};
};

} // namespace tui

#endif // TUI_DESKTOP_DESKTOP_MANAGER_HPP
