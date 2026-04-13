#include "platform/terminal.hpp"
#include "platform/input.hpp"
#include "ui/renderer.hpp"
#include "ui/panel.hpp"
#include "ui/label.hpp"
#include "ui/list.hpp"
#include "desktop/desktop_manager.hpp"
#include "window/window_manager.hpp"
#include "core/event.hpp"
#include "core/colors.hpp"
#include "core/string_utils.hpp"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>

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
          desktop_mgr_(3)  // Start with 3 desktops
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
        if (!term_->init()) {
            std::cerr << "Failed to initialize terminal\n";
            return false;
        }

        term_->enter_alternate_screen();
        term_->enter_raw_mode();
        term_->cursor_hide();
        term_->set_title("Desktop TUI");

        // Initialize input
        input_->init();

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
            destroy_terminal(term_);
            term_ = nullptr;
        }
        if (input_) {
            destroy_input(input_);
            input_ = nullptr;
        }
    }

    // Main event loop
    void run() {
        renderer_.mark_dirty();

        while (g_running) {
            // Check for pending resize (signal-driven)
            int new_cols = 0, new_rows = 0;
            if (term_->check_resize(new_cols, new_rows)) {
                renderer_.resize(new_cols, new_rows);
                desktop_mgr_.on_resize(new_cols, new_rows);
                renderer_.mark_dirty();
            }

            // Process all pending input events
            while (auto event = poll_input()) {
                if (!handle_event(*event)) break;
            }

            // Render
            render();

            // Small sleep to prevent 100% CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60fps
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
        }

        // Resize events
        if (e.type == EventType::Resize) {
            renderer_.resize(e.cols, e.rows);
            desktop_mgr_.on_resize(e.cols, e.rows);
            renderer_.mark_dirty();
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
            renderer_.write_center(mid_y - 2, welcome, cols, Styles::Title());
            renderer_.write_center(mid_y, hint1, cols, Styles::Info());
            renderer_.write_center(mid_y + 1, hint2, cols, Styles::Info());
            renderer_.write_center(mid_y + 2, hint3, cols, Styles::Info());
            renderer_.write_center(mid_y + 4, hint4, cols, Styles::Dim());
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

    ITerminal* term_;
    IInput* input_;
    Renderer renderer_;
    DesktopManager desktop_mgr_;
};

int main() {
    TUIShell shell;

    if (!shell.init()) {
        std::cerr << "Failed to initialize Desktop TUI\n";
        return 1;
    }

    shell.run();

    return 0;
}
