#ifndef TUI_DESKTOP_DESKTOP_HPP
#define TUI_DESKTOP_DESKTOP_HPP

#include "window/window.hpp"
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <algorithm>

namespace tui {

using DesktopId = uint32_t;

/// A virtual desktop containing windows
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
        windows_.push_back(std::move(win));
    }

    void remove_window(WindowId id) {
        windows_.erase(
            std::remove_if(windows_.begin(), windows_.end(),
                [id](const auto& w) { return w->id() == id; }),
            windows_.end()
        );
    }

    // Get windows on this desktop
    const std::vector<std::shared_ptr<Window>>& windows() const { return windows_; }

    // Find window by ID
    Window* find_window(WindowId id) {
        for (auto& w : windows_) {
            if (w->id() == id) return w.get();
        }
        return nullptr;
    }

    bool has_window(WindowId id) const {
        return std::any_of(windows_.begin(), windows_.end(),
            [id](const auto& w) { return w->id() == id; });
    }

    int window_count() const { return static_cast<int>(windows_.size()); }

    // Move a window to another desktop
    void move_to_desktop(WindowId id, Desktop& target) {
        auto it = std::find_if(windows_.begin(), windows_.end(),
            [id](const auto& w) { return w->id() == id; });
        if (it != windows_.end()) {
            auto win = *it;
            windows_.erase(it);
            target.add_window(win);
        }
    }

private:
    DesktopId id_;
    std::string name_;
    bool active_ = false;
    std::vector<std::shared_ptr<Window>> windows_;

    static DesktopId next_id_;
};

} // namespace tui

#endif // TUI_DESKTOP_DESKTOP_HPP
