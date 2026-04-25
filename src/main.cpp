#include "platform/terminal.hpp"
#include "platform/input.hpp"
#include "ui/renderer.hpp"
#include "ui/panel.hpp"
#include "ui/label.hpp"
#include "ui/list.hpp"
#include "desktop/desktop_manager.hpp"
#include "core/event.hpp"
#include "core/colors.hpp"
#include "core/string_utils.hpp"
#include "core/config.hpp"
#include "version.hpp"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include <cstring>

using namespace tui;

// Global for signal handling
static std::atomic<bool> g_running{true};

void signal_handler(int) {
    g_running = false;
}

/// TUI Shell - the main application controller
class TUIShell {
public:
    TUIShell()
        : term_(create_terminal()),
          input_(create_input()),
          renderer_(*term_),
          desktop_mgr_(Config::DEFAULT_DESKTOP_COUNT)  // Start with configured default desktops
    {}

    ~TUIShell() {
        shutdown();
    }

    bool init() {
        // Setup signal handlers
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // Emergency cleanup on exit
        std::atexit([]() {
            // This runs if normal cleanup doesn't complete
            // Just in case, try to restore terminal
            printf("\033[0m\033[?25h\033[?1049l");
            fflush(stdout);
        });

        // Initialize terminal
        if (!term_ || !term_->init()) {
            std::cerr << "Failed to initialize terminal\n";
            return false;
        }

        term_->enter_alternate_screen();
        term_->enter_raw_mode();
        term_->cursor_hide();
        term_->set_title("Desktop TUI");

        // Initialize input
        if (input_) {
            input_->init();
        }

        // Initialize renderer
        renderer_.init();
        renderer_.clear();

        // Setup desktop switch callback
        desktop_mgr_.on_switched([this](Desktop&) {
            renderer_.mark_dirty();
        });

        // Create demo windows on desktop 1
        setup_demo();

        return true;
    }

    void shutdown() {
        if (term_) {
            term_->cursor_show();
            term_->leave_raw_mode();
            term_->leave_alternate_screen();
            term_->shutdown();
            term_.reset();
        }
        if (input_) {
            input_.reset();
        }
    }

    // Main event loop
    void run() {
        renderer_.mark_dirty();
        bool needs_render = true;

        while (g_running) {
            // Check for pending resize (signal-driven)
            int new_cols = 0, new_rows = 0;
            if (term_->check_resize(new_cols, new_rows)) {
                renderer_.resize(new_cols, new_rows);
                desktop_mgr_.on_resize(new_cols, new_rows);
                needs_render = true;
            }

            // Process all pending input events
            bool had_input = false;
            while (auto event = poll_input()) {
                if (!handle_event(*event)) break;
                had_input = true;
            }

            // Only render if something changed
            if (had_input || needs_render) {
                render();
                needs_render = false;
            } else {
                // Sleep when idle — configured duration gives ~20fps wake-up for input checks
                std::this_thread::sleep_for(Config::IDLE_SLEEP_DURATION);
            }
        }
    }

private:
    std::optional<Event> poll_input() {
        if (input_->has_input()) {
            return input_->poll();
        }
        return std::nullopt;
    }

    bool handle_event(const Event& e) {
        // Global keybindings
        if (e.type == EventType::KeyPress) {
            // Ctrl+Q: Quit
            if (e.mods.control && (e.key_code == 'Q' || e.key_code == 'q')) {
                g_running = false;
                return false;
            }

            // Alt+1..9: Switch desktop
            if (e.mods.alt && e.key_code >= '1' && e.key_code <= '9') {
                int idx = e.key_code - '1';
                desktop_mgr_.switch_to(idx);
                return true;
            }

            // Alt+Left/Right: Prev/Next desktop
            if (e.mods.alt && e.key_code == Keys::ArrowLeft) {
                desktop_mgr_.switch_prev();
                return true;
            }
            if (e.mods.alt && e.key_code == Keys::ArrowRight) {
                desktop_mgr_.switch_next();
                return true;
            }

            // Alt+N: New desktop
            if (e.mods.alt && (e.key_code == 'N' || e.key_code == 'n')) {
                int count = desktop_mgr_.desktop_count() + 1;
                desktop_mgr_.add_desktop("Desktop " + std::to_string(count));
                return true;
            }

            // Alt+W: Close focused window
            if (e.mods.alt && (e.key_code == 'W' || e.key_code == 'w')) {
                auto* active = desktop_mgr_.active_desktop();
                if (active) {
                    for (auto& win : active->windows()) {
                        if (win->is_focused()) {
                            active->remove_window(win->id());
                            break;
                        }
                    }
                }
                return true;
            }

            // Alt+Tab: Cycle windows
            if (e.mods.alt && e.key_code == Keys::Tab) {
                auto* active = desktop_mgr_.active_desktop();
                if (active) {
                    bool found_focused = false;
                    Window* next_win = nullptr;
                    for (auto& win : active->windows()) {
                        if (found_focused && win->visible()) {
                            next_win = win.get();
                            break;
                        }
                        if (win->is_focused()) found_focused = true;
                    }
                    if (!next_win && !active->windows().empty()) {
                        next_win = active->windows().front().get();
                    }
                    if (next_win) {
                        for (auto& win : active->windows()) {
                            win->blur();
                        }
                        next_win->focus();
                    }
                }
                return true;
            }

            // Escape: minimize focused window
            if (e.key_code == Keys::Escape) {
                auto* active = desktop_mgr_.active_desktop();
                if (active) {
                    for (auto& win : active->windows()) {
                        if (win->is_focused()) {
                            win->minimize();
                            break;
                        }
                    }
                }
                return true;
            }

            // F11: maximize/restore focused window
            if (e.key_code == Keys::F11 || e.key_name == "F11") {
                auto* active = desktop_mgr_.active_desktop();
                if (active) {
                    for (auto& win : active->windows()) {
                        if (win->is_focused()) {
                            if (win->is_maximized()) {
                                win->restore_maximized();
                            } else {
                                win->maximize();
                            }
                            break;
                        }
                    }
                }
                return true;
            }

            // FIX C5: Alt+R restore minimized windows
            if (e.mods.alt && (e.key_code == 'R' || e.key_code == 'r')) {
                auto* active = desktop_mgr_.active_desktop();
                if (active) {
                    for (auto& win : active->windows()) {
                        if (win->is_minimized()) {
                            win->restore();
                            win->focus();
                            break;
                        }
                    }
                }
                return true;
            }
        }

        // Resize events
        if (e.type == EventType::Resize) {
            renderer_.resize(e.cols, e.rows);
            desktop_mgr_.on_resize(e.cols, e.rows);
            renderer_.mark_dirty();
            return true;
        }

        // Mouse events - handle window dragging and resizing
        if (e.type == EventType::MouseDown) {
            auto* active = desktop_mgr_.active_desktop();
            if (active) {
                // Find focused window
                for (auto& win : active->windows()) {
                    if (win->is_focused() && win->visible()) {
                        // Check if clicking on resize handle
                        int mx = e.mouse_x, my = e.mouse_y;
                        bool on_handle = false;

                        // Right handle (last 2 columns, bottom row)
                        if (win->bounds().w > 20 && mx >= win->bounds().x + win->bounds().w - 2 && my >= win->bounds().y && my < win->bounds().y + win->bounds().h) {
                            on_handle = true;
                            win->start_resize(1);
                        }
                        // Bottom handle (last row, left 2 columns)
                        else if (win->bounds().h > 5 && mx >= win->bounds().x && mx < win->bounds().x + win->bounds().w - 2 && my >= win->bounds().y + win->bounds().h - 1) {
                            on_handle = true;
                            win->start_resize(2);
                        }
                        // Both handles (corner)
                        else if (win->bounds().w > 20 && win->bounds().h > 5 && mx >= win->bounds().x + win->bounds().w - 2 && my >= win->bounds().y + win->bounds().h - 1) {
                            on_handle = true;
                            win->start_resize(3);
                        }

                        if (on_handle) {
                            return true;
                        }

                        // Otherwise start dragging
                        if (win->is_dragging() == false) {
                            win->start_drag();
                            win->set_drag_offset(e.mouse_x - win->bounds_x(), e.mouse_y - win->bounds_y());
                        }
                        break;
                    }
                }
            }
            return true;
        }

        if (e.type == EventType::MouseMove) {
            // Update mouse position for dragging or resizing
            auto* active = desktop_mgr_.active_desktop();
            if (active) {
                for (auto& win : active->windows()) {
                    if (win->is_dragging()) {
                        win->move(e.mouse_x - win->drag_offset_x(), e.mouse_y - win->drag_offset_y());
                        return true;
                    }
                    if (win->is_resizing()) {
                        if (win->resize_dir() & 1) {
                            win->set_bounds_w(e.mouse_x - win->bounds_x() + 1);
                        }
                        if (win->resize_dir() & 2) {
                            win->set_bounds_h(e.mouse_y - win->bounds_y() + 1);
                        }
                        return true;
                    }
                }
            }
            return true;
        }

        if (e.type == EventType::MouseUp) {
            auto* active = desktop_mgr_.active_desktop();
            if (active) {
                for (auto& win : active->windows()) {
                    if (win->is_dragging()) {
                        win->end_drag();
                        return true;
                    }
                    if (win->is_resizing()) {
                        win->end_resize();
                        return true;
                    }
                }
            }
            return true;
        }

        // Dispatch to active desktop's focused window
        desktop_mgr_.dispatch_event(e);

        return true;
    }

    void render() {
        renderer_.clear();

        int cols = renderer_.cols();
        int rows = renderer_.rows();

        // Draw top bar
        Style bar_style = Style::Default();
        bar_style.bg = Color::Pal(4);
        bar_style.fg = Color::RGB(255, 255, 255);
        bar_style.bold = true;

        std::string bar_text = " Desktop TUI ";
        renderer_.write(0, 0, repeat(bar_text, cols / static_cast<int>(bar_text.size()) + 1), bar_style);

        // Desktop name in center
        auto* active = desktop_mgr_.active_desktop();
        if (active) {
            std::string name = " " + active->name() + " ";
            int x = (cols - static_cast<int>(name.size())) / 2;
            renderer_.write(x, 0, name, bar_style);
        }

        // Help text on right
        std::string help = "Ctrl+Q:Quit ";
        renderer_.write_right(cols - 1, 0, help, cols, bar_style);

        // Status bar at bottom
        Style status_style = Style::Default();
        status_style.bg = Color::Pal(235);
        status_style.fg = Color::Pal(248);

        std::string status;
        if (active) {
            status = " " + std::to_string(active->window_count()) + " window(s) ";
        }
        renderer_.write(0, rows - 1, status, status_style);

        // Desktop indicator (workspace dots)
        desktop_mgr_.render_indicator(renderer_, rows - 1);

        // Maximize indicator
        if (active) {
            for (auto& win : active->windows()) {
                if (win->is_maximized()) {
                    status += " [Maximized] ";
                    break;
                }
            }
        }
        renderer_.write(0, rows - 1, status, status_style);

        // Render active desktop windows
        desktop_mgr_.render_active_desktop(renderer_);

        // If no windows, show welcome message
        if (active && active->window_count() == 0) {
            std::string welcome = "Welcome to Desktop TUI";
            std::string hint1 = "Alt+N - New desktop";
            std::string hint2 = "Alt+1..9 - Switch desktop";
            std::string hint3 = "Alt+Left/Right - Navigate desktops";
            std::string hint4 = "Ctrl+Q - Quit";

            int mid_y = rows / 2;
            renderer_.write_center(0, mid_y - 2, welcome, cols, Styles::Title());
            renderer_.write_center(0, mid_y, hint1, cols, Styles::Info());
            renderer_.write_center(0, mid_y + 1, hint2, cols, Styles::Info());
            renderer_.write_center(0, mid_y + 2, hint3, cols, Styles::Info());
            renderer_.write_center(0, mid_y + 4, hint4, cols, Styles::Dim());
        }

        renderer_.flush();
    }

    void setup_demo() {
        // Create a demo window
        auto win1 = std::make_shared<Window>("Welcome", Rect{2, 2, 50, 10});
        auto panel1 = std::make_shared<Panel>("Info", Styles::BorderActive());
        auto label = std::make_shared<Label>(
            "Desktop TUI v0.1\n"
            "A cross-platform multi-desktop TUI\n"
            "written in pure C++17.\n\n"
            "No dependencies. Maximum portability.",
            Style::Default()
        );
        label->set_bounds({3, 3, 46, 6});
        panel1->add_child(label);
        win1->set_content(panel1);

        auto* active = desktop_mgr_.active_desktop();
        if (active) {
            active->add_window(win1);
            win1->focus();
        }

        // Create a list demo window
        auto win2 = std::make_shared<Window>("Shortcuts", Rect{55, 2, 30, 14});
        auto panel2 = std::make_shared<Panel>("Keybindings", Styles::Border());
        auto list = std::make_shared<List>();
        list->set_items({
            "Ctrl+Q: Quit",
            "Alt+1..9: Desktop",
            "Alt+Left/Right: Nav",
            "Alt+N: New desktop",
            "Alt+Tab: Cycle windows",
            "Alt+W: Close window",
            "Esc: Minimize",
            "Arrows: Navigate"
        });
        list->set_bounds({1, 1, 28, 8});
        panel2->add_child(list);
        win2->set_content(panel2);

        if (active) {
            active->add_window(win2);
        }
    }

    std::unique_ptr<ITerminal> term_;
    std::unique_ptr<IInput> input_;
    Renderer renderer_;
    DesktopManager desktop_mgr_;
};

int main(int argc, char* argv[]) {
    // Handle --version and --help flags
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--version") == 0 || std::strcmp(argv[i], "-v") == 0) {
            std::printf("Desktop TUI v%s\n", TUI_VERSION);
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
            std::printf("Usage: desktop-tui [OPTIONS]\n");
            std::printf("\nA cross-platform multi-desktop TUI written in C++17.\n");
            std::printf("\nOptions:\n");
            std::printf("  --version, -v    Show version and exit\n");
            std::printf("  --help, -h       Show this help and exit\n");
            std::printf("\nKeybindings:\n");
            std::printf("  Ctrl+Q           Quit\n");
            std::printf("  Alt+1..9         Switch to desktop N\n");
            std::printf("  Alt+Left/Right   Previous/Next desktop\n");
            std::printf("  Alt+N            New desktop\n");
            std::printf("  Alt+Tab          Cycle windows\n");
            std::printf("  Alt+W            Close focused window\n");
            std::printf("  Alt+R            Restore minimized window\n");
            std::printf("  Esc              Minimize focused window\n");
            return 0;
        }
    }

    TUIShell shell;

    if (!shell.init()) {
        std::cerr << "Failed to initialize Desktop TUI\n";
        return 1;
    }

    shell.run();

    return 0;
}
