#ifndef TUI_HPP
#define TUI_HPP

/// Desktop TUI - A cross-platform multi-desktop TUI in C++17
///
/// Single include entry point. Include this header to access all TUI components.

// Core (consolidated via common.hpp)
#include "core/common.hpp"
#include "core/signal.hpp"

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

// Desktop
#include "desktop/desktop.hpp"
#include "desktop/desktop_manager.hpp"

#endif // TUI_HPP
