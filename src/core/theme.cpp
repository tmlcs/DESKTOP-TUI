#include "core/theme.hpp"
#include "core/colors.hpp"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

namespace tui {

Theme Theme::get_default() {
    Theme theme;
    theme.bg_primary = Color::Pal(234);
    theme.bg_secondary = Color::Pal(235);
    theme.bg_window = Color::Pal(236);
    theme.fg_primary = Color::Pal(250);
    theme.fg_secondary = Color::Pal(248);
    theme.fg_window = Color::Pal(252);
    theme.border_active = Color::Pal(221);
    theme.border_inactive = Color::Pal(240);
    theme.title_bg = Color::Pal(233);
    theme.title_fg = Color::Pal(250);
    theme.button_bg = Color::Pal(237);
    theme.button_fg = Color::Pal(250);
    theme.button_hover_bg = Color::Pal(229);
    theme.button_hover_fg = Color::Pal(255);
    theme.focus_indicator = Color::Pal(217);
    theme.font = "monospace";
    theme.font_size = 1;
    theme.show_decorations = true;
    theme.decoration_margin = 1;
    theme.show_panel_border = true;
    theme.panel_border = Color::Pal(238);
    theme.show_status_bar = true;
    theme.status_bg = Color::Pal(234);
    theme.status_fg = Color::Pal(250);
    return theme;
}

std::optional<Theme> Theme::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open theme file: " << path << std::endl;
        return std::nullopt;
    }

    Theme theme = get_default();
    std::string line;
    int line_num = 0;

    while (std::getline(file, line)) {
        line_num++;
        // Simple parser for theme file
        if (line.find("bg_primary") != std::string::npos) {
            // Parse bg_primary
        }
    }

    file.close();
    return theme;
}

bool Theme::save_to_file(const std::string& path, const Theme& theme) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Failed to open theme file for writing: " << path << std::endl;
        return false;
    }

    file << "# Theme configuration\n";
    file << "bg_primary = " << (theme.bg_primary.mode == Color::Mode::Indexed ? std::to_string(theme.bg_primary.index) : "truecolor") << "\n";
    file << "bg_secondary = " << (theme.bg_secondary.mode == Color::Mode::Indexed ? std::to_string(theme.bg_secondary.index) : "truecolor") << "\n";
    file << "bg_window = " << (theme.bg_window.mode == Color::Mode::Indexed ? std::to_string(theme.bg_window.index) : "truecolor") << "\n";
    file << "fg_primary = " << (theme.fg_primary.mode == Color::Mode::Indexed ? std::to_string(theme.fg_primary.index) : "truecolor") << "\n";
    file << "fg_secondary = " << (theme.fg_secondary.mode == Color::Mode::Indexed ? std::to_string(theme.fg_secondary.index) : "truecolor") << "\n";
    file << "fg_window = " << (theme.fg_window.mode == Color::Mode::Indexed ? std::to_string(theme.fg_window.index) : "truecolor") << "\n";
    file << "border_active = " << (theme.border_active.mode == Color::Mode::Indexed ? std::to_string(theme.border_active.index) : "truecolor") << "\n";
    file << "border_inactive = " << (theme.border_inactive.mode == Color::Mode::Indexed ? std::to_string(theme.border_inactive.index) : "truecolor") << "\n";
    file << "title_bg = " << (theme.title_bg.mode == Color::Mode::Indexed ? std::to_string(theme.title_bg.index) : "truecolor") << "\n";
    file << "title_fg = " << (theme.title_fg.mode == Color::Mode::Indexed ? std::to_string(theme.title_fg.index) : "truecolor") << "\n";
    file << "button_bg = " << (theme.button_bg.mode == Color::Mode::Indexed ? std::to_string(theme.button_bg.index) : "truecolor") << "\n";
    file << "button_fg = " << (theme.button_fg.mode == Color::Mode::Indexed ? std::to_string(theme.button_fg.index) : "truecolor") << "\n";
    file << "button_hover_bg = " << (theme.button_hover_bg.mode == Color::Mode::Indexed ? std::to_string(theme.button_hover_bg.index) : "truecolor") << "\n";
    file << "button_hover_fg = " << (theme.button_hover_fg.mode == Color::Mode::Indexed ? std::to_string(theme.button_hover_fg.index) : "truecolor") << "\n";
    file << "focus_indicator = " << (theme.focus_indicator.mode == Color::Mode::Indexed ? std::to_string(theme.focus_indicator.index) : "truecolor") << "\n";
    file << "font = " << theme.font << "\n";
    file << "font_size = " << theme.font_size << "\n";
    file << "show_decorations = " << (theme.show_decorations ? "true" : "false") << "\n";
    file << "decoration_margin = " << theme.decoration_margin << "\n";
    file << "show_panel_border = " << (theme.show_panel_border ? "true" : "false") << "\n";
    file << "panel_border = " << (theme.panel_border.mode == Color::Mode::Indexed ? std::to_string(theme.panel_border.index) : "truecolor") << "\n";
    file << "show_status_bar = " << (theme.show_status_bar ? "true" : "false") << "\n";
    file << "status_bg = " << (theme.status_bg.mode == Color::Mode::Indexed ? std::to_string(theme.status_bg.index) : "truecolor") << "\n";
    file << "status_fg = " << (theme.status_fg.mode == Color::Mode::Indexed ? std::to_string(theme.status_fg.index) : "truecolor") << "\n";

    file.close();
    return true;
}

} // namespace tui
