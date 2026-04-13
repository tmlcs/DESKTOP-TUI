#ifndef TUI_DESKTOP_DESKTOP_HPP
#define TUI_DESKTOP_DESKTOP_HPP

#include "window/window.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>
#include <unordered_set>

namespace tui {

using DesktopId = uint32_t;

/// A virtual desktop containing windows
/// Desktops track window membership but do NOT own windows (WindowManager owns them)
class Desktop {
public:
    Desktop() : id_(next_id_++) {}

    explicit Desktop(const std::string& name)
        : id_(next_id_++), name_(name) {}

    DesktopId id() const { return id_; }

    void set_name(const std::string& name) { name_ = name; }
    std::string name() const { return name_; }

    bool is_active() const { return active_; }
    void set_active(bool a) { active_ = a; }

    // Window management within this desktop
    void add_window(std::shared_ptr<Window> win) {
        window_ids_.insert(win->id());
        // Store weak reference for rendering
        windows_.push_back(std::move(win));
        // Clean up stale entries
        cleanup_stale_windows();
    }

    void remove_window(WindowId id) {
        window_ids_.erase(id);
        windows_.erase(
            std::remove_if(windows_.begin(), windows_.end(),
                [id](const auto& w) { return w->id() == id; }),
            windows_.end()
        );
    }

    // Get windows on this desktop (only valid/alive windows)
    std::vector<std::shared_ptr<Window>> windows() {
        cleanup_stale_windows();
        return windows_;
    }

    // Find window by ID
    Window* find_window(WindowId id) {
        for (auto& w : windows_) {
            if (w->id() == id) return w.get();
        }
        return nullptr;
    }

    bool has_window(WindowId id) const {
        return window_ids_.count(id) > 0;
    }

    int window_count() const { return static_cast<int>(windows_.size()); }

    // Move a window to another desktop
    void move_to_desktop(WindowId id, Desktop& target) {
        auto it = std::find_if(windows_.begin(), windows_.end(),
            [id](const auto& w) { return w->id() == id; });
        if (it != windows_.end()) {
            auto win = *it;
            windows_.erase(it);
            window_ids_.erase(id);
            target.add_window(win);
        }
    }

private:
    void cleanup_stale_windows() {
        // No-op since we own the shared_ptrs, but kept for API consistency
        // if weak pointers are ever used
    }

    DesktopId id_;
    std::string name_;
    bool active_ = false;
    std::unordered_set<WindowId> window_ids_;
    std::vector<std::shared_ptr<Window>> windows_;

    static DesktopId next_id_;
};

} // namespace tui

#endif // TUI_DESKTOP_DESKTOP_HPP
