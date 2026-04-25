# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Test

### Standard Build
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./desktop-tui
```

### With Tests
```bash
mkdir build && cd build
cmake .. -DENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest              # Run all tests
./test_all --string_utils   # Run specific test
```

### With Sanitizers (Debug)
```bash
mkdir build && cd build
cmake .. -DENABLE_ASAN=ON -DENABLE_UBSAN=ON -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Plugin Demo
```bash
mkdir build && cd build
cmake .. -DENABLE_PLUGINS=ON
make -j$(nproc)
./plugin_demo --help
```

## Architecture

### Single-Include Design
The entire framework is accessible via `#include "tui.hpp"`. Core subsystems are header-only with implementations in `src/`:

- **Core** (`include/core/`): `Rect`, `Color`/`Style`, `Event`, `Signal<T>`, `EventBus`, `Config` — all header-only
- **UI** (`include/ui/`): `Renderer`, `Widget`, `Panel`, `Label`, `List`, `TextInput` — headers in `include/`, impl in `src/ui/`
- **Desktop** (`include/desktop/`): `Desktop`, `DesktopManager` — headers in `include/`, impl in `src/desktop/`
- **Window** (`include/window/`): `Window` — header in `include/`, impl in `src/window/`
- **Platform** (`src/platform/`): Platform-specific `ITerminal` and `IInput` implementations (POSIX, Windows, Android, Generic)

### Application Flow
```
┌─────────────────────────────────────────────────────┐
│                    TUIShell                          │
│              (Main loop, event dispatch, render)    │
├─────────────────────────────────────────────────────┤
│  TUIShell::run()                                     │
│    ├─ Poll input events                               │
│    ├─ Dispatch to focused window                       │
│    └─ Render (dirty-region optimized)                 │
├─────────────────────────────────────────────────────┤
│  DesktopManager                                       │
│    ├─ Virtual desktops (configurable count)           │
│    ├─ Window management (z-order, focus, minimize)    │
│    └─ Desktop switching (Alt+1..9, arrows)           │
├─────────────────────────────────────────────────────┤
│  WindowManager                                         │
│    ├─ Window bounds, dragging, resizing               │
│    ├─ Maximize/restore                                 │
│    └─ Event dispatch to focused window                │
├─────────────────────────────────────────────────────┤
│  Renderer (double-buffered, dirty-region)            │
│    ├─ Front/back buffers                               │
│    ├─ UTF-8 with box-drawing, 256/true color         │
│    └─ Zero-allocation flush() (P1-01 optimization)   │
├─────────────────────────────────────────────────────┤
│  Platform Abstraction Layer                          │
│    ├─ ITerminal: termios/Console API                  │
│    ├─ IInput: stdin/ReadConsoleInput                  │
│    └─ SGR 1006 mouse events                           │
├─────────────────────────────────────────────────────┤
│  Core Utilities                                       │
│    ├─ Event, Signal, EventBus                         │
│    ├─ Rect, Color, Style                               │
│    └─ String, Logger, Config                           │
└─────────────────────────────────────────────────────┘
```

### Key Design Patterns
- **Double-buffered rendering**: `Renderer` maintains front/back buffers with dirty-region optimization
- **Event-driven loop**: Main loop in `src/main.cpp` polls input, dispatches to focused window, renders on change
- **Platform abstraction**: `ITerminal` and `IInput` interfaces with platform-specific factories (`create_terminal()`, `create_input()`)
- **Signal/slot**: `Signal<T>` for direct connections, `EventBus` for publish/subscribe (both single-threaded only)
- **Zero-allocation rendering**: P1-01 optimization uses pre-allocated buffers to eliminate runtime allocations

### Thread Safety Notes
- `EventBus`, `Signal<T>`, `Renderer`, `Style`, `TextInput` are **NOT thread-safe** — designed for single-threaded UI loop
- `ClipboardImpl` is thread-safe and exposed for concurrent access testing
- `Style` uses bitfields; concurrent modification of the same instance is unsafe

## Configuration

Centralized in `include/core/config.hpp`:
- `DEFAULT_DESKTOP_COUNT=3`, `MAX_DESKTOP_COUNT=20`
- `IDLE_SLEEP_DURATION=50ms` (~20fps wake-up)
- `MIN_WINDOW_WIDTH=20`, `MIN_WINDOW_HEIGHT=5`
- `MAX_TITLE_LENGTH=256` (security)
- `MAX_TERMINAL_COLS=1024`, `MAX_TERMINAL_ROWS=1024`
- `ENABLE_DIRTY_REGION_OPTIMIZATION=true`
- `ENABLE_DOUBLE_BUFFERING=true`
- `ENABLE_TILING=true` (snap-to-edge with 10px threshold)

## Keybindings

| Key | Action |
|-----|-|
| `Ctrl+Q` | Quit |
| `Alt+1..9` | Switch to desktop N |
| `Alt+←/→` | Previous/Next desktop |
| `Alt+N` | New desktop |
| `Alt+Tab` | Cycle windows |
| `Alt+W` | Close focused window |
| `Alt+R` | Restore minimized window |
| `Esc` | Minimize focused window |
| `F11` | Maximize/restore focused window |

## Platform Support Matrix

| Platform | Build Define | Terminal | Input | Notes |
|------|------|----------|-------|-------|
| Linux | `TUI_PLATFORM_LINUX` | termios + VT100 | stdin + SGR 1006 | Full feature support |
| macOS | `TUI_PLATFORM_MACOS` | termios + VT100 | stdin + SGR 1006 | Full feature support |
| Windows | `TUI_PLATFORM_WINDOWS` | Console API / VT | ReadConsoleInput | VT on Win10+, fallback on older |
| Android | `TUI_PLATFORM_ANDROID` | termux + VT100 | stdin + SGR 1006 | Same as Linux |
| Generic | `TUI_PLATFORM_GENERIC` | VT100 fallback | getchar() | Minimal support |

## Terminal Compatibility

| Terminal | 256 Color | True Color | Mouse | Box Drawing |
|------|-----------|---------|-------|-------|
| xterm | ✓ | ✓ | ✓ | ✓ |
| Alacritty | ✓ | ✓ | ✓ | ✓ |
| Kitty | ✓ | ✓ | ✓ | ✓ |
| GNOME Terminal | ✓ | ✓ | ✓ | ✓ |
| Windows Terminal | ✓ | ✓ | ✓ | ✓ |
| Termux | ✓ | ✓ | ✓ | ✓ |
| tmux | ✓ | ✓* | ✓ | ✓ |

*Requires `set -g default-terminal "tmux-256color"` and `set -ga terminal-overrides ",*:Tc"`

## Plugin System

Dynamic plugin loading via `dlopen`/`LoadLibrary`. Plugins must export:
```cpp
extern "C" IPlugin* create_plugin();
extern "C" void destroy_plugin(IPlugin*);
```

Plugin interface (`include/plugins/plugin_interface.hpp`):
- `IPlugin::get_info()` — Plugin metadata
- `IPlugin::initialize(PluginContext&)` — Initialize with host context
- `IPlugin::execute()` — Execute plugin (re-callable)
- `IPlugin::shutdown()` — Cleanup resources
- `IPlugin::handle_command()` — Custom commands

Capabilities system (sandboxing):
- `ReadConfig`, `WriteConfig`, `SpawnProcess`, `AccessClipboard`
- `AccessNetwork`, `AccessFiles`, `AccessHardware`
- `CapabilityDetector` validates requested capabilities

Build with `-DENABLE_PLUGINS=ON`. Example plugins in `examples/plugins/`.

## Version

Read from `VERSION` file (current: 0.3.0), embedded via CMake `configure_file()` into `version.hpp`.

## Security Features

- **Bracketed Paste Injection Prevention**: Validates bracketed paste sequences before processing
- **Title Sanitization**: Strips control characters from terminal titles
- **Input Buffer Limits**: 64KB max buffer to prevent DoS
- **Terminal Dimension Limits**: 1024x1024 max to prevent memory exhaustion

## Test Organization

Tests in `tests/`:
- `test_main.cpp` — Test entry point with argument parsing
- `test_string_utils.cpp` — String utility functions
- `test_renderer.cpp` — Renderer functionality
- `test_critical_fixes.cpp` — Regression tests for known issues
- `test_thread_safety.cpp` — Concurrent access validation
- `test_rect_safety.cpp` — Geometry bounds checking
- `test_desktop_manager.cpp` — Virtual desktop management
- `test_braille_renderer.cpp` — Braille character rendering
- `test_capability_detector.cpp` — Plugin capability validation
- `test_integration.cpp` — End-to-end integration tests

Run specific tests: `./test_all --<test_name>`

## Performance Benchmarks

Benchmark utilities in `tests/benchmark.cpp`:
- Flush performance (zero-allocation validation)
- Rendering throughput
- Event loop latency

## Design Principles

1. **Zero Dependencies**: No ncurses, no external libraries
2. **C++17 Only**: Modern features (std::optional, std::variant, structured bindings)
3. **RAII**: Resource management via smart pointers and destructors
4. **Platform Abstraction**: Clean interface per platform, compiled conditionally
5. **Double-Buffered Rendering**: Front/back buffers with dirty-region optimization
6. **Event-Driven**: Non-blocking input polling, event bus dispatch
7. **Security First**: Input validation, buffer limits, sanitization

## License

MIT
