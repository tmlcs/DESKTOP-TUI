// demo_hello.cpp - Simple "Hello World" TUI application
// Demonstrates basic window creation and rendering

#include "../include/desktop/desktop_manager.hpp"
#include "../include/ui/panel.hpp"
#include "../include/ui/label.hpp"
#include "../include/ui/button.hpp"
#include "../include/platform/terminal.hpp"
#include "../include/platform/input.hpp"
#include "../include/core/event.hpp"
#include "../include/core/colors.hpp"
#include "../include/core/config.hpp"

#include <iostream>
#include <string>

using namespace tui;

int main(int argc, char* argv[]) {
    // Create terminal
    auto term = create_terminal();
    if (!term->init()) {
        std::cerr << "Failed to initialize terminal\n";
        return 1;
    }

    term->enter_alternate_screen();
    term->enter_raw_mode();
    term->cursor_hide();

    // Create input handler
    auto input = create_input();
    input->init();

    // Create renderer
    Renderer renderer(*term);
    renderer.init();
    renderer.clear();

    // Create desktop manager
    DesktopManager dm(1);

    // Create a simple "Hello" window
    auto win = std::make_shared<Window>("Hello", Rect{1, 1, 79, 24});
    auto panel = std::make_shared<Panel>("Hello World", Styles::Border());

    auto label = std::make_shared<Label>(
        "Hello from Desktop TUI!\n\n"
        "A simple C++17 TUI application.\n"
        "No dependencies, maximum portability.\n\n"
        "Press Ctrl+Q to quit.",
        Style::Default()
    );
    label->set_bounds({2, 2, 75, 18});
    panel->add_child(label);

    win->set_content(panel);
    dm.active_desktop()->add_window(win);

    // Main loop
    bool running = true;
    while (running) {
        // Check for resize
        int new_cols = 0, new_rows = 0;
        if (term->check_resize(new_cols, new_rows)) {
            renderer.resize(new_cols, new_rows);
            dm.on_resize(new_cols, new_rows);
        }

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
            }
        }

        // Render
        renderer.clear();
        renderer.write_center(0, 0, " Hello ", 81, Style::Default());
        renderer.write_center(0, 24, " Ctrl+Q to quit ", 81, Style::Default());

        // Draw desktop indicator
        dm.render_indicator(renderer, 24);

        // Render windows
        dm.render_active_desktop(renderer);

        renderer.flush();

        // Sleep to avoid busy loop
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Cleanup
    term->cursor_show();
    term->leave_raw_mode();
    term->leave_alternate_screen();
    term->shutdown();
    destroy_terminal(term);
    destroy_input(input);

    return 0;
}
