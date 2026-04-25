// demo_multiwindow.cpp - Multi-window demonstration
// Shows window creation, positioning, and basic interactions

#include "../include/desktop/desktop_manager.hpp"
#include "../include/ui/panel.hpp"
#include "../include/ui/label.hpp"
#include "../include/ui/button.hpp"
#include "../include/ui/text_input.hpp"
#include "../include/ui/list.hpp"
#include "../include/platform/terminal.hpp"
#include "../include/platform/input.hpp"
#include "../include/core/event.hpp"
#include "../include/core/colors.hpp"

#include <iostream>
#include <string>
#include <thread>

using namespace tui;

void create_demo_windows(DesktopManager& dm) {
    auto* desktop = dm.active_desktop();
    if (!desktop) return;

    // Create title bar helper
    auto title_bar = [](auto& p, const std::string& title) {
        auto lbl = std::make_shared<Label>(title, Style::Default());
        lbl->set_bounds({1, 1, p->width(), 1});
        p->add_child(lbl);
        return p;
    };

    // 1. Info window
    auto win1 = std::make_shared<Window>("Info", {0, 0, 40, 15});
    auto panel1 = std::make_shared<Panel>("Info", Styles::BorderActive());
    auto info1 = std::make_shared<Label>(
        "Desktop TUI v0.1\n"
        "Multi-window demo",
        Style::Default()
    );
    info1->set_bounds({2, 2, 36, 11});
    panel1->add_child(info1);
    win1->set_content(panel1);
    desktop->add_window(win1);

    // 2. Keybindings window
    auto win2 = std::make_shared<Window>("Keybindings", {45, 0, 35, 15});
    auto panel2 = std::make_shared<Panel>("Keybindings", Styles::Border());
    auto lb2 = std::make_shared<List>({
        {"Ctrl+Q", "Quit"},
        {"Alt+1-9", "Switch desktop"},
        {"Alt+Left/Right", "Navigate desktops"},
        {"Alt+N", "New desktop"},
        {"Alt+Tab", "Cycle windows"},
        {"Alt+W", "Close window"},
        {"Esc", "Minimize"},
        {"Arrows", "Navigate windows"}
    });
    lb2->set_bounds({1, 1, 33, 13});
    panel2->add_child(lb2);
    win2->set_content(panel2);
    desktop->add_window(win2);

    // 3. Welcome panel (hidden initially)
    auto win3 = std::make_shared<Window>("Welcome", {15, 10, 20, 8});
    auto panel3 = std::make_shared<Panel>("Welcome", Styles::DimBorder());
    auto lbl3 = std::make_shared<Label>(
        "No more windows\n"
        "Alt+N: New desktop",
        Style::Dim()
    );
    lbl3->set_bounds({3, 3, 14, 4});
    panel3->add_child(lbl3);
    win3->set_content(panel3);
    desktop->add_window(win3);
}

int main() {
    auto term = create_terminal();
    if (!term->init()) return 1;
    term->enter_alternate_screen();
    term->enter_raw_mode();
    term->cursor_hide();

    auto input = create_input();
    input->init();

    Renderer renderer(*term);
    renderer.init();
    renderer.clear();

    DesktopManager dm(3);  // Create 3 desktops
    dm.on_switched([](Desktop&) { renderer.mark_dirty(); });

    create_demo_windows(dm);

    int last_win = -1;  // Track currently selected window

    bool running = true;
    while (running) {
        // Process input
        while (auto event = input->poll()) {
            if (event->type == EventType::KeyPress) {
                if (event->mods.control && (event->key_code == 'Q' || event->key_code == 'q')) {
                    running = false;
                    break;
                }
                if (event->mods.escape) {
                    running = false;
                    break;
                }

                // Window cycling (Alt+1-3 or Alt+Tab)
                if (event->mods.alt) {
                    if (event->key_code >= '1' && event->key_code <= '3') {
                        int idx = event->key_code - '1';
                        if (idx < 3) {
                            auto* win = dm.active_desktop()->find_window(idx + 1);
                            if (win) {
                                // Find and blur all windows
                                for (auto& w : dm.active_desktop()->windows()) {
                                    w->blur();
                                }
                                win->focus();
                                last_win = idx;
                            }
                        }
                    }
                    if (event->key_code == Keys::Tab) {
                        // Cycle to next window
                        auto* desktop = dm.active_desktop();
                        if (desktop) {
                            bool found = false;
                            for (auto& win : desktop->windows()) {
                                if (!win->is_focused()) {
                                    for (auto& w : desktop->windows()) {
                                        w->blur();
                                    }
                                    win->focus();
                                    last_win++;
                                    if (last_win >= static_cast<int>(desktop->windows().size())) {
                                        last_win = -1;
                                    }
                                    found = true;
                                    break;
                                }
                            }
                        }
                    }
                }

                // Window closing (Alt+W)
                if (event->mods.alt && (event->key_code == 'W' || event->key_code == 'w')) {
                    auto* desktop = dm.active_desktop();
                    if (desktop) {
                        for (auto& win : desktop->windows()) {
                            if (win->is_focused()) {
                                desktop->remove_window(win->id());
                                break;
                            }
                        }
                    }
                }
            }
            else if (event->type == EventType::Resize) {
                renderer.resize(event->cols, event->rows);
                dm.on_resize(event->cols, event->rows);
            }
        }

        // Render
        renderer.clear();

        // Top bar
        Style bar_style = Style::Default();
        bar_style.bg = Color::Pal(4);
        bar_style.fg = Color::RGB(255, 255, 255);
        bar_style.bold = true;
        renderer.write(0, 0, " Desktop TUI ", renderer.cols(), bar_style);

        auto* desktop = dm.active_desktop();
        if (desktop) {
            renderer.write_center(0, 0, " " + desktop->name() + " ", renderer.cols(), bar_style);
        }

        // Help text
        renderer.write_right(renderer.cols() - 1, 0, "Ctrl+Q:Quit", renderer.cols(), bar_style);

        // Render windows
        dm.render_active_desktop(renderer);

        // Desktop indicator
        dm.render_indicator(renderer, renderer.rows() - 1);

        // Status bar
        Style status_style = Style::Default();
        status_style.bg = Color::Pal(235);
        status_style.fg = Color::Pal(248);
        std::string status = " " + std::to_string(desktop ? desktop->window_count() : 0) + " window(s) ";
        renderer.write(0, renderer.rows() - 1, status, status_style);

        renderer.flush();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    term->cursor_show();
    term->leave_raw_mode();
    term->leave_alternate_screen();
    term->shutdown();
    destroy_terminal(term);
    destroy_input(input);

    return 0;
}
