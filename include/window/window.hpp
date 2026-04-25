#ifndef TUI_WINDOW_WINDOW_HPP
#define TUI_WINDOW_WINDOW_HPP

#include "core/common.hpp"
#include "ui/widget.hpp"
#include "ui/panel.hpp"
#include "ui/context_menu.hpp"
#include "core/rect.hpp"
#include "core/event.hpp"
#include "core/colors.hpp"
#include "core/string_utils.hpp"
#include "core/config.hpp"
#include "core/theme.hpp"
#include <string>
#include <memory>
#include <functional>

namespace tui {

using WindowId = uint64_t;

/// A window in the window system
///
/// @note OWNERSHIP MODEL:
///       - Desktop owns windows via std::vector<std::shared_ptr<Window>>
///       - DesktopManager owns desktops via std::vector<std::shared_ptr<Desktop>>
///       - When a Desktop is removed, its windows are transferred to the active Desktop
///       - This ownership model ensures proper cleanup and prevents dangling pointers
///
///       DesktopManager creates and owns Desktop instances.
///       Each Desktop owns its windows.
///       When a Desktop is removed, its windows are transferred to the active Desktop.
class Window {
public:
    Window() : id_(next_id_++) {}

    explicit Window(const std::string& title, Rect bounds = {})
        : id_(next_id_++), title_(title), bounds_(bounds) {
        if (bounds_.w == 0) bounds_.w = Config::MIN_WINDOW_WIDTH * 10;  // Use config constant
        if (bounds_.h == 0) bounds_.h = Config::MIN_WINDOW_HEIGHT * 6;   // Use config constant
    }

    WindowId id() const { return id_; }

    // Properties
    void set_title(const std::string& title) { title_ = title; }
    std::string title() const { return title_; }

    // Welcome window marker (FIX C7)
    void set_welcome() { welcome_ = true; }
    bool is_welcome() const { return welcome_; }

    void set_bounds(Rect r) { bounds_ = r; on_move(); }
    Rect bounds() const { return bounds_; }

    void move(int x, int y) { bounds_.x = x; bounds_.y = y; on_move(); }
    void resize(int w, int h) { bounds_.w = std::max(0, w); bounds_.h = std::max(0, h); on_resize(); }

    // Visibility and state
    void show() { visible_ = true; }
    void hide() { visible_ = false; }
    bool visible() const { return visible_ && !minimized_; }
    bool is_minimized() const { return minimized_; }
    bool is_modal() const { return is_modal_; }
    void set_modal(bool modal) { is_modal_ = modal; }

    void minimize() {
        minimized_ = true;
        emit_state_changed();
    }

    void restore() {
        minimized_ = false;
        emit_state_changed();
    }

    void toggle_minimize() { minimized_ = !minimized_; }

    // Maximize state
    void maximize() {
        maximized_ = true;
        previous_bounds_ = bounds_;
        bounds_ = Rect{0, 0, 0, 0};  // Will be set to desktop size by manager
        emit_state_changed();
    }

    void restore_maximized() {
        maximized_ = false;
        bounds_ = previous_bounds_;
        emit_state_changed();
    }

    bool is_maximized() const { return maximized_; }

    // Focus
    void focus() {
        focused_ = true;
        // Blur other windows on the same desktop (focus isolation)
        // This is handled by DesktopManager in real usage
    }
    void blur() { focused_ = false; }
    bool is_focused() const { return focused_; }

    // Drag state
    void start_drag() { dragging_ = true; }
    void end_drag() { dragging_ = false; }
    bool is_dragging() const { return dragging_; }

    // Drag getters/setters (for external use)
    void set_drag_offset(int x, int y) { drag_offset_x_ = x; drag_offset_y_ = y; }
    int drag_offset_x() const { return drag_offset_x_; }
    int drag_offset_y() const { return drag_offset_y_; }

    // Resize state
    void start_resize(int dir) { resizing_ = true; resize_dir_ = dir; }
    void end_resize() { resizing_ = false; }
    bool is_resizing() const { return resizing_; }

    // Resize getters/setters (for external use)
    int resize_dir() const { return resize_dir_; }
    void set_resize_dir(int dir) { resize_dir_ = dir; }

    // Tiling state
    void set_tiled(bool tiled) { is_tiled_ = tiled; }
    void set_tile_group_id(uint32_t id) { tile_group_id_ = id; }
    bool is_tiled() const { return is_tiled_; }
    uint32_t tile_group_id() const { return tile_group_id_; }
    void set_tile_mode(Config::TileMode mode) { tile_mode_ = mode; }
    Config::TileMode tile_mode() const { return tile_mode_; }

    // Bounds accessors (for external use)
    int bounds_x() const { return bounds_.x; }
    int bounds_y() const { return bounds_.y; }
    int bounds_w() const { return bounds_.w; }
    int bounds_h() const { return bounds_.h; }
    void set_bounds_w(int w) { bounds_.w = w; }
    void set_bounds_h(int h) { bounds_.h = h; }

    // Content - the window contains a root panel
    void set_content(std::shared_ptr<Panel> panel) { content_ = std::move(panel); }
    std::shared_ptr<Panel> content() const { return content_; }

    // Focusable state
    bool focusable_ = false;
    void set_focusable(bool focusable) { focusable_ = focusable; }
    bool is_focusable() const { return focusable_; }

    // Render the window
    void render(class Renderer& r) {
        if (!visible_ || minimized_) return;

        if (maximized_) {
            // Draw maximized window covering entire screen
            int cols = r.cols();
            int rows = r.rows();
            bounds_ = Rect{0, 0, cols, rows};
            if (content_) {
                content_->set_bounds(bounds_);
                content_->render(r);
            } else {
                Style border = focused_ ? Styles::BorderActive() : Styles::Border();
                r.draw_box(bounds_, border, Style::Default());
                // Title centered at top
                if (rows > 4) {
                    std::string title = " " + title_ + " ";
                    int max_w = cols - 2;
                    if (static_cast<int>(tui::display_width(title)) > max_w) {
                        title = tui::truncate(title, static_cast<size_t>(max_w));
                    }
                    int title_dw = static_cast<int>(tui::display_width(title));
                    int title_x = (cols - title_dw) / 2;
                    r.write(title_x, 1, title, Styles::Title());
                }
            }
        } else if (content_) {
            content_->set_bounds(bounds_);
            content_->render(r);
        } else {
            // Default: draw window frame
            Style border = focused_ ? Styles::BorderActive() : Styles::Border();
            r.draw_box(bounds_, border, Style::Default());

            // Title
            if (bounds_.w > 4) {
                // Truncate title to fit width (UTF-8 safe)
                std::string title = " " + title_ + " ";
                int max_w = bounds_.w - 6; // leave margins + buttons
                if (static_cast<int>(tui::display_width(title)) > max_w) {
                    title = tui::truncate(title, static_cast<size_t>(max_w));
                }
                int title_dw = static_cast<int>(tui::display_width(title));
                int title_x = bounds_.x + (bounds_.w - title_dw - 4) / 2;
                r.write(title_x, bounds_.y, title, Styles::Title());

                // Draw window control buttons (close, minimize, maximize)
                if (bounds_.w > 12) {
                    // Close button (X)
                    r.write(bounds_.x + bounds_.w - 3, bounds_.y, "X", Styles::Normal());
                    // Minimize button (-)
                    r.write(bounds_.x + bounds_.w - 6, bounds_.y, "-", Styles::Normal());
                    // Maximize button ([])
                    if (!maximized_) {
                        r.write(bounds_.x + bounds_.w - 9, bounds_.y, "[", Styles::Normal());
                        r.write(bounds_.x + bounds_.w - 8, bounds_.y, "]", Styles::Normal());
                    } else {
                        r.write(bounds_.x + bounds_.w - 9, bounds_.y, "[]", Styles::Normal());
                    }
                }
            }

            // Draw resize handles when resizing
            if (resizing_) {
                Style handle_style = Styles::Normal();
                handle_style.bg = Color::Pal(236);
                handle_style.fg = Color::Pal(255);
                handle_style.underline = true;

                // Right handle
                if (resize_dir_ & 1 && bounds_.w > Config::RESIZE_HANDLE_RIGHT) {
                    r.write(bounds_.x + bounds_.w - Config::RESIZE_HANDLE_RIGHT, bounds_.y + 1, ">", handle_style);
                }
                // Bottom handle
                if (resize_dir_ & 2 && bounds_.h > Config::MIN_WINDOW_HEIGHT) {
                    r.write(bounds_.x + 1, bounds_.y + bounds_.h - 1, "v", handle_style);
                }
                // Both handles
                if (resize_dir_ == 3 && bounds_.w > Config::MIN_WINDOW_WIDTH && bounds_.h > Config::MIN_WINDOW_HEIGHT) {
                    r.write(bounds_.x + bounds_.w - Config::RESIZE_HANDLE_RIGHT, bounds_.y + 1, ">", handle_style);
                    r.write(bounds_.x + 1, bounds_.y + bounds_.h - 1, "v", handle_style);
                }
            }
        }
    }

    // Event handling
    bool handle_event(const Event& e) {
        if (!visible_ || minimized_) return false;

        // Handle drag events
        if (dragging_) {
            if (e.type == EventType::MouseMove) {
                move(e.mouse_x - drag_offset_x_, e.mouse_y - drag_offset_y_);
                return true;
            }
            if (e.type == EventType::MouseUp) {
                end_drag();
                return true;
            }
        }

        // Handle resize events
        if (resizing_) {
            if (e.type == EventType::MouseMove) {
                if (resize_dir_ & 1) { // Right handle
                    bounds_.w = e.mouse_x - bounds_.x + 1;
                }
                if (resize_dir_ & 2) { // Bottom handle
                    bounds_.h = e.mouse_y - bounds_.y + 1;
                }
                return true;
            }
            if (e.type == EventType::MouseUp) {
                end_resize();
                return true;
            }
        }

        // Handle maximize events
        if (e.type == EventType::KeyPress) {
            if (e.key_code == Keys::F11 || e.key_name == "F11") {
                if (maximized_) {
                    restore_maximized();
                } else {
                    maximize();
                }
                return true;
            }
        }

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

    // State change callback
    std::function<void()> on_state_changed;

protected:
    void on_move() {
        if (content_) content_->set_bounds(bounds_);
    }

    void on_resize() {
        if (content_) content_->set_bounds(bounds_);
    }

    void emit_state_changed() {
        if (on_state_changed) on_state_changed();
    }

    WindowId id_;
    std::string title_;
    Rect bounds_;
    bool visible_ = true;
    bool focused_ = false;
    bool minimized_ = false;
    bool welcome_ = false;  // Welcome window marker (FIX C7)
    bool is_modal_ = false;  // Modal overlay marker (FIX C6)
    bool dragging_ = false;
    int drag_offset_x_ = 0;
    int drag_offset_y_ = 0;
    bool resizing_ = false;
    int resize_dir_ = 0;  // 0=none, 1=right, 2=bottom, 3=both
    bool maximized_ = false;
    Rect previous_bounds_;
    std::shared_ptr<Panel> content_;

    // Tiling state
    bool is_tiled_ = false;  // Window is in tile mode
    uint32_t tile_group_id_ = 0;  // Group ID for tiled windows
    Config::TileMode tile_mode_ = Config::DEFAULT_TILE_MODE;  // Current tile layout

    // Context menu state
    ContextMenu* context_menu_ = nullptr;

    static WindowId next_id_;

    static WindowId init_next_id() { return 0; }
};

inline WindowId Window::next_id_ = 0;

} // namespace tui

#endif // TUI_WINDOW_WINDOW_HPP
