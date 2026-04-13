#ifndef TUI_WINDOW_WINDOW_MANAGER_HPP
#define TUI_WINDOW_WINDOW_MANAGER_HPP

#include "window.hpp"
#include "ui/renderer.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <optional>
#include <functional>

namespace tui {

/// Manages windows: creation, z-order, focus, lifecycle
class WindowManager {
public:
    using WindowPtr = std::shared_ptr<Window>;

    WindowManager() = default;

    // Create a new window
    WindowPtr create_window(const std::string& title, Rect bounds = {}) {
        auto win = std::make_shared<Window>(title, bounds);
        windows_.push_back(win);
        update_z_order();
        focus_window(win.get());
        on_window_created_(*win);
        return win;
    }

    // Close a window
    void close_window(WindowId id) {
        auto it = std::find_if(windows_.begin(), windows_.end(),
            [id](const auto& w) { return w->id() == id; });
        if (it != windows_.end()) {
            bool was_focused = (*it)->is_focused();
            on_window_closing_(*(*it));
            windows_.erase(it);
            update_z_order();
            if (was_focused) {
                // Focus the top window
                if (!windows_.empty()) {
                    focus_window(windows_.back().get());
                }
            }
        }
    }

    // Close all windows
    void close_all() {
        windows_.clear();
        focused_ = nullptr;
    }

    // Find window by ID
    Window* find_window(WindowId id) {
        for (auto& w : windows_) {
            if (w->id() == id) return w.get();
        }
        return nullptr;
    }

    // Get focused window
    Window* focused_window() { return focused_; }

    // Focus a window (brings to front)
    void focus_window(Window* win) {
        if (!win) return;
        if (focused_) focused_->blur();
        focused_ = win;
        focused_->focus();

        // Bring to front
        auto it = std::find_if(windows_.begin(), windows_.end(),
            [win](const auto& w) { return w.get() == win; });
        if (it != windows_.end()) {
            windows_.erase(it);
            windows_.push_back(*it);
            update_z_order();
        }
    }

    // Focus next window (Alt+Tab behavior)
    void focus_next() {
        if (windows_.empty()) return;
        auto it = std::find_if(windows_.begin(), windows_.end(),
            [this](const auto& w) { return w.get() == focused_; });
        if (it == windows_.end() || it == windows_.end() - 1) {
            focus_window(windows_.front().get());
        } else {
            focus_window((*(it + 1)).get());
        }
    }

    // Focus previous window
    void focus_prev() {
        if (windows_.empty()) return;
        auto it = std::find_if(windows_.begin(), windows_.end(),
            [this](const auto& w) { return w.get() == focused_; });
        if (it == windows_.end() || it == windows_.begin()) {
            focus_window(windows_.back().get());
        } else {
            focus_window((*(it - 1)).get());
        }
    }

    // Minimize a window
    void minimize_window(Window* win) {
        if (!win) return;
        win->minimize();
        if (focused_ == win) focus_next();
    }

    // Get all visible windows (sorted by z-order)
    std::vector<Window*> visible_windows() {
        std::vector<Window*> result;
        for (auto& w : windows_) {
            if (w->visible()) result.push_back(w.get());
        }
        return result;
    }

    // Get all windows
    const std::vector<WindowPtr>& all_windows() const { return windows_; }

    int window_count() const { return static_cast<int>(windows_.size()); }

    // Render all visible windows in z-order
    void render(Renderer& r) {
        for (auto& win : windows_) {
            if (win->visible()) {
                win->render(r);
            }
        }
    }

    // Dispatch event to focused window
    bool dispatch_event(const Event& e) {
        if (focused_ && focused_->visible()) {
            return focused_->handle_event(e);
        }
        return false;
    }

    // Callbacks
    void on_window_created(std::function<void(Window&)> cb) { on_window_created_ = std::move(cb); }
    void on_window_closing(std::function<void(Window&)> cb) { on_window_closing_ = std::move(cb); }

    // Resize all windows
    void on_resize(int cols, int rows) {
        for (auto& win : windows_) {
            win->on_resize(cols, rows);
        }
    }

private:
    void update_z_order() {
        int z = 0;
        for (auto& win : windows_) {
            win->set_z_order(z++);
        }
    }

    std::vector<WindowPtr> windows_;
    Window* focused_ = nullptr;

    std::function<void(Window&)> on_window_created_ = [](Window&){};
    std::function<void(Window&)> on_window_closing_ = [](Window&){};
};

} // namespace tui

#endif // TUI_WINDOW_WINDOW_MANAGER_HPP
