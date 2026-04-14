# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.5] - 2026-04-14

### Refactored

- **DRY: `emit_style()` extracted to shared function (D1)**: New `emit_style_to_string(const Style&)` in `colors.hpp` replaces ~120 duplicated lines across 4 terminal implementations (`terminal_posix.cpp`, `terminal_win.cpp`, `terminal_generic.cpp`, `renderer.hpp`). All terminals now delegate to the single shared implementation.
- **DRY: `draw_box()` default implementation (D2)**: Moved `draw_box()` logic to a default implementation in `ITerminal` interface (`terminal.hpp`), eliminating 3 near-identical copies across platform-specific terminal files.
- **Box character consolidation (D3/D4)**: Box drawing now uses `has_cap(TerminalCaps::BoxDrawing)` consistently across all platforms, with Unicode/ASCII selection handled at the capability level rather than duplicated per-terminal.

### Added

- **16 new unit tests**: 10 string_utils edge case tests (empty inputs, zero-width truncation, whitespace trimming) and 6 `emit_style_to_string` correctness tests (default, bold, italic, 256-color, truecolor FG/BG). Total: 56 passing tests.

## [0.1.4] - 2026-04-14

### Fixed

- **DesktopManager initializes active desktop (C2)**: Both constructors now call `switch_to(0)` after creating desktops, ensuring the app works correctly on launch. Previously `active_` was null until the user pressed Alt+1..9 or Alt+arrows.
- **remove_desktop() index corruption (C3)**: Removing a desktop before the active one now correctly decrements `active_index_`, preventing the indicator from highlighting the wrong dot.
- **remove_desktop() dangling pointer (C4)**: Removing the active desktop now updates the `active_` pointer to the desktop that replaces it, preventing crash on subsequent `active_desktop()` dereference.
- **UTF-8 corruption in Renderer::write() (C1)**: Text rendering now iterates by UTF-8 codepoint using `utf8_decode()` instead of byte-by-byte, preventing multi-byte character corruption during flush.
- **UTF-8 corruption in pad/center/right_align (C1)**: String alignment functions now use `display_width()` instead of `s.size()` for padding calculations, preventing mid-codepoint truncation.
- **UTF-8 corruption in word_wrap (C1)**: Word wrapping now measures line width using `display_width()` instead of byte count.
- **UTF-8 corruption in List widget (C1)**: List item truncation now uses `truncate()` with display width instead of `substr()` with byte count.
- **UTF-8 corruption in Panel title (C1)**: Panel title centering now uses `display_width()` for position calculation.
- **Escape key permanently loses window (C5)**: Added `Alt+R` keybinding to restore the most recently minimized window.
- **Panel renders children without clipping (C6)**: Child widgets are now checked against the panel's content area using `Rect::intersection()` before rendering, preventing overflow into adjacent UI elements.
- **Signal/EventBus iterator invalidation (C7)**: `Signal::emit()` and `EventBus::publish()` now snapshot their handler lists before iterating, preventing crashes if a callback disconnects itself during dispatch.
- **Compiler warning: uninitialized current_style**: `Renderer::flush()` now initializes `current_style` to `Style::Default()`.
- **Compiler warning: unused mark_dirty params**: Unused `x` and `w` parameters in `mark_dirty(x,y,w,h)` are now properly commented out.
- **Compiler warning: unused variable in truncate()**: Removed unused `before` variable from `truncate()`.

### Removed

- **Dead code removal (C8)**: Removed `WindowManager` (160 lines), `EventBus` class from `event.hpp` (45 lines), and `Signal` class from `signal.hpp` (45 lines). These were never instantiated and added maintenance burden.
- **Removed WindowManager files from build**: `window_manager.hpp` and `window_manager.cpp` deleted from source tree and CMakeLists.txt.

### Added

- **18 new unit tests** covering DesktopManager initialization, remove_desktop() correctness, UTF-8 rendering, pad/center/right_align display width, word_wrap, List truncation, Panel clipping, Signal/EventBus iterator safety, and Alt+R restore keybinding. Total: 40 passing tests.

## [0.1.3] - 2026-04-14

### Fixed

- **Renderer bounds checking**: `clear_region()` and `fill_rect()` now clamp loop ranges with `std::max(0, ...)` / `std::min(cols_, ...)` eliminating unnecessary iterations over negative coordinates.
- **UTF-8 safe window titles**: Window title rendering now uses `display_width()` and `truncate()` for proper Unicode handling. Long CJK titles no longer break centering or get truncated mid-character.
- **Desktop resize propagation**: `Desktop::on_resize()` now repositions off-screen windows when terminal size changes. `DesktopManager::on_resize()` calls both desktop and window level resize handlers.
- **Mouse scroll wheel support**: SGR 1006 mouse parser now detects scroll wheel events (button >= 64) and emits `EventType::MouseScroll` with proper `scroll_delta` values for up/down/left/right.
- **EOF handling**: When stdin returns 0 bytes (terminal closed), input now emits `EventType::Quit` instead of silently returning `std::nullopt`.
- **Idle CPU reduction**: Main loop idle sleep reduced from 100ms to 50ms for more responsive input wake-ups.

### Added

- **`mark_dirty(Rect)`**: Renderer now supports marking specific regions dirty instead of full-screen redraw, enabling targeted re-rendering optimization.
- **Unit test framework**: 40 tests covering `string_utils` (display_width, truncate, trim, split, pad/center/right_align) and `renderer` (bounds checking for put, write, fill_rect, clear_region, draw_box, write_right, write_center, mark_dirty_rect).
- **CMake test support**: `ENABLE_TESTS` option to build unit tests. `ENABLE_ASAN` and `ENABLE_UBSAN` options for sanitizer builds.
- **Version header generation**: `version.hpp.in` template auto-generates `TUI_VERSION`, `TUI_VERSION_MAJOR`, `TUI_VERSION_MINOR`, `TUI_VERSION_PATCH` macros at build time.
- **`install()` target**: `make install` now installs binary to `CMAKE_INSTALL_PREFIX/bin`.

### Changed

- **Style documentation**: Added `static_assert` and comment documenting that `Style` is single-threaded only (bitfields share storage).

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
