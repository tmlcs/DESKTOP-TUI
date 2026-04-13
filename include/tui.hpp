#ifndef TUI_HPP
#define TUI_HPP

/// Desktop TUI - A cross-platform multi-desktop TUI in C++17
///
/// Single include entry point. Include this header to access all TUI components.

// Core
#include "core/event.hpp"
#include "core/signal.hpp"
#include "core/rect.hpp"
#include "core/colors.hpp"
#include "core/string_utils.hpp"

// Platform
#include "platform/terminal.hpp"
#include "platform/input.hpp"

// UI
#include "ui/renderer.hpp"
#include "ui/widget.hpp"
#include "ui/panel.hpp"
#include "ui/label.hpp"
#include "ui/border.hpp"
#include "ui/list.hpp"

// Window
#include "window/window.hpp"
#include "window/window_manager.hpp"

// Desktop
#include "desktop/desktop.hpp"
#include "desktop/desktop_manager.hpp"

#endif // TUI_HPP
