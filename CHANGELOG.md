# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.2] - 2026-04-14

### Fixed

- **Use-after-free in WindowManager**: `focus_window()` now saves the `shared_ptr` before erasing from vector, preventing iterator dereference after erase (UB/crash on Alt+Tab).
- **Buffer overflow in terminal style emission**: `emit_style()` now clamps `snprintf` accumulated length before writing to 64-byte stack buffer.
- **Uninitialized terminal state**: `orig_termios_` is now initialized in the constructor. `tcgetattr` return value is validated in `enter_raw_mode()` — failure prevents raw mode activation. Terminal destructor now clears the global signal handler pointer to prevent dangling access.
- **`trim()` undefined behavior**: Fixed iterator decrement past `begin()` on empty strings. Added early return and safe loop bounds.
- **`write_right` coordinate calculation**: Fixed double-addition of `bounds_.w` that placed right-aligned text far off-screen.
- **`write_center` ignores widget offset**: Now accepts a `base_x` parameter to correctly center text within widgets positioned at non-zero screen coordinates.
- **Generic platform input blocking**: `has_input()` now returns `false` instead of always `true`, preventing the main loop from blocking indefinitely on `getchar()`.
- **Panel child clipping**: Children are now checked for intersection with the panel's content area before rendering, preventing overflow into adjacent UI elements.
- **Deprecated `std::codecvt` removed**: Replaced with a manual UTF-8 decoder (`utf8_decode`). `display_width()` and `truncate()` now operate directly on UTF-8 bytes without deprecated library functions. `truncate()` now correctly compares display width instead of codepoint count.
- **Windows `draw_hline` garbage output**: Fixed `std::string(w, char)` truncating 3-byte UTF-8 box-drawing character to a single byte. Now concatenates properly like POSIX version.
- **Windows `fill()` ignores style**: Non-VT mode now applies color attributes via `FillConsoleOutputAttribute`. Bold maps to `FOREGROUND_INTENSITY`.
- **Windows `emit_style` missing flags**: Added `blink` (5) and `hidden` (8) style attributes to match POSIX implementation.
- **100% CPU on idle**: Main loop now only renders when input was received or dirty flag is set. Sleeps 100ms during idle instead of spinning at 16ms intervals.
- **`repeat()` performance**: Added `reserve()` to prevent O(n²) reallocations.

## [0.1.1] - 2026-04-14

### Fixed

- **SIGWINCH resize detection**: Terminal now properly detects window resize events via signal-driven flag polling. `ITerminal::check_resize()` method added for main loop to query pending resizes.
- **Front buffer reset on resize**: Renderer now resets the front buffer when terminal dimensions change, preventing visual artifacts from stale cells.
- **Bounds checking in text alignment**: `write_center()` and `write_right()` now clamp coordinates and truncate text that exceeds the available width, preventing out-of-bounds writes.
- **Robust input buffering**: Input parser now uses a byte buffer to handle fragmented escape sequences. Partial CSI/SS3 sequences are accumulated across poll calls instead of being discarded.
- **Mouse event propagation to children**: `Panel::handle_event()` now performs hit-testing for mouse events, giving focus to clicked child widgets before dispatching the event.
- **`move_window_to_desktop` functional**: DesktopManager can now correctly move windows between desktops by finding the source desktop, removing the window, and adding it to the target.
- **Emergency terminal cleanup**: `atexit()` handler now restores terminal state (reset style, show cursor, exit alternate screen) if the process exits abnormally.
- **CJK wide character width**: `display_width()` now correctly counts 2 cells for CJK, Hangul, Hiragana, Katakana, and fullwidth characters.

### Changed

- `Desktop::windows()` now returns by value (copy) instead of const reference, with internal cleanup of stale entries.
- `Desktop` now tracks window IDs in an `unordered_set` for O(1) `has_window()` lookups.

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
