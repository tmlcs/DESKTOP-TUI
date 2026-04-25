#ifndef TUI_CORE_THEME_HPP
#define TUI_CORE_THEME_HPP

#include "core/colors.hpp"
#include "core/config.hpp"
#include <string>
#include <map>
#include <optional>

namespace tui {

/// Theme configuration for the TUI framework
/// Supports CSS-like JSON config files for colors, fonts, and styles
struct Theme {
    // Background colors
    Color bg_primary = Color::Pal(234);   // Main background
    Color bg_secondary = Color::Pal(235); // Secondary background
    Color bg_window = Color::Pal(236);     // Window background

    // Foreground colors
    Color fg_primary = Color::Pal(250);    // Main foreground
    Color fg_secondary = Color::Pal(248);  // Secondary foreground
    Color fg_window = Color::Pal(252);     // Window foreground

    // Border colors
    Color border_active = Color::Pal(221); // Active window border
    Color border_inactive = Color::Pal(240); // Inactive window border

    // Title bar colors
    Color title_bg = Color::Pal(233);      // Title bar background
    Color title_fg = Color::Pal(250);      // Title bar foreground

    // Button colors
    Color button_bg = Color::Pal(237);     // Button background
    Color button_fg = Color::Pal(250);     // Button foreground
    Color button_hover_bg = Color::Pal(229); // Hover state
    Color button_hover_fg = Color::Pal(255); // Hover foreground

    // Focus indicator
    Color focus_indicator = Color::Pal(217); // Focus ring/indicator

    // Font settings
    std::string font = "monospace";        // Default font
    int font_size = 1;                     // Font size (terminal rows)

    // Window decorations
    bool show_decorations = true;           // Show window decorations
    int decoration_margin = 1;              // Margin around decorations

    // Panel settings
    bool show_panel_border = true;          // Show panel borders
    Color panel_border = Color::Pal(238);   // Panel border color

    // Status bar
    bool show_status_bar = true;            // Show status bar
    Color status_bg = Color::Pal(234);      // Status bar background
    Color status_fg = Color::Pal(250);      // Status bar foreground

    // Load theme from file (JSON-like format)
    static std::optional<Theme> load_from_file(const std::string& path);

    // Save theme to file
    static bool save_to_file(const std::string& path, const Theme& theme);

    // Get default theme
    static Theme get_default();
};

} // namespace tui

#endif // TUI_CORE_THEME_HPP
