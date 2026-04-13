# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2026-04-13

### Added

#### Core
- Event bus with type-safe event dispatch and wildcard subscriptions
- Signal/slot system for lightweight UI callbacks
- Rectangle geometry system (Point, Rect) with intersection, clamp, expand
- Color system supporting 256-color palette and 24-bit true color
- Text styling (bold, italic, underline, blink, reverse, dim, hidden)
- UTF-8 string utilities (split, trim, pad, center, wrap, truncate)

#### Platform Abstraction
- Cross-platform terminal interface (`ITerminal`) with capability detection
- POSIX terminal implementation (Linux, macOS) using termios + VT100
- Windows terminal implementation using Console API with VT fallback
- Android/Termux terminal support (shares POSIX implementation)
- Generic fallback terminal for unknown platforms
- Cross-platform input handler (`IInput`) with non-blocking poll
- Keyboard input parsing (escape sequences, function keys, modifiers)
- Mouse input via SGR 1006 protocol
- SIGWINCH resize event handling

#### Rendering Engine
- Double-buffered rendering with front/back cell buffers
- Dirty-region optimization (only redraw changed cells)
- Per-cell style tracking with style-run batching
- UTF-8 character encoding for output
- Box-drawing character support (Unicode + ASCII fallback)
- Auto-detect terminal capabilities (256-color, true-color, mouse, unicode)

#### Widget System
- Base `Widget` class with bounds, visibility, focus, event handling
- `Panel` container with optional border, title, and child management
- `Label` widget with text wrapping and alignment (left, center, right)
- `Border` widget for standalone bordered rectangles
- `List` widget with scroll, selection, and keyboard navigation

#### Window System
- `Window` model with title, bounds, visibility, focus, minimize/restore
- `WindowManager` with z-order stack, focus tracking, lifecycle callbacks
- Window operations: create, close, move, resize, minimize, focus, cycle
- Alt+Tab window cycling with wrap-around
- Window content via embedded `Panel` hierarchy

#### Virtual Desktop System
- `Desktop` model with independent window sets
- `DesktopManager` with multiple desktops and switching
- Desktop operations: add, remove, switch by index or navigation
- Window assignment tracking per desktop
- Desktop indicator bar (workspace dots with state visualization)

#### TUI Shell
- Main event loop with non-blocking input polling (~60fps)
- Global keybindings (Ctrl+Q, Alt+1..9, Alt+arrows, Alt+N, Alt+W, Alt+Tab, Esc)
- Top bar with title and desktop name
- Bottom status bar with window count
- Welcome screen with keybinding hints (no windows)
- Demo windows on startup (welcome panel + shortcuts list)

#### Build System
- CMake 3.14+ with C++17 requirement
- Automatic platform detection and conditional compilation
- Zero external dependencies (stdlib + platform APIs only)
- Install target configuration

### Changed

- N/A (initial release)

### Deprecated

- N/A (initial release)

### Removed

- N/A (initial release)

### Fixed

- N/A (initial release)

### Security

- N/A (initial release)

---

[Unreleased]: https://github.com/tmlcs/desktop-tui/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/tmlcs/desktop-tui/releases/tag/v0.1.0
