# Desktop TUI

A cross-platform, zero-dependency multi-desktop TUI (Text User Interface) written in pure C++17.

## Features

- **Virtual Desktops**: Multiple workspaces with window management
- **Window System**: Floating windows with z-order, focus, minimize/restore
- **ANSI/Unicode Rendering**: Box-drawing characters, 256-color, and true color support
- **Double-Buffered Rendering**: Dirty-region optimization for smooth updates
- **Cross-Platform**: Linux, macOS, Windows, Android (Termux), and generic fallback
- **Zero Dependencies**: Only C++ standard library + platform APIs
- **Mouse Support**: SGR 1006 mouse events

## Keybindings

| Key | Action |
|-----|--------|
| `Ctrl+Q` | Quit |
| `Alt+1..9` | Switch to desktop N |
| `Alt+←/→` | Previous/Next desktop |
| `Alt+N` | New desktop |
| `Alt+Tab` | Cycle windows |
| `Alt+W` | Close focused window |
| `Esc` | Minimize focused window |

## Building

### Requirements

- C++17 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.14+

### Linux/macOS

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
./desktop-tui
```

### Windows (MSVC)

```cmd
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
.\Release\desktop-tui.exe
```

### Android (Termux)

```bash
pkg install cmake make clang
mkdir build && cd build
cmake ..
make -j$(nproc)
./desktop-tui
```

## Architecture

```
┌─────────────────────────────────────────────────┐
│                   TUI Shell                      │
│  (Main loop, event dispatch, render pipeline)    │
├──────────────┬──────────────┬───────────────────┤
│  Desktop Mgr │  Window Mgr  │   Input Handler   │
├──────────────┴──────────────┴───────────────────┤
│              Platform Abstraction Layer          │
│  (Terminal I/O, Signals, Filesystem, Time)       │
├─────────────────────────────────────────────────┤
│              Core Utilities                       │
│  (String, Math, Events, Observer, Logger)        │
└─────────────────────────────────────────────────┘
```

### Platform Support

| Platform | Terminal | Input | Notes |
|----------|----------|-------|-------|
| Linux | termios + VT100 | stdin + SGR mouse | Full feature support |
| macOS | termios + VT100 | stdin + SGR mouse | Full feature support |
| Windows | Console API / VT | ReadConsoleInput | VT on Win10+, fallback on older |
| Android | termux + VT100 | stdin + SGR mouse | Same as Linux |
| Generic | VT100 fallback | getchar() | Minimal support |

### Terminal Compatibility

| Terminal | 256 Color | True Color | Mouse | Box Drawing |
|----------|-----------|------------|-------|-------------|
| xterm | ✓ | ✓ | ✓ | ✓ |
| Alacritty | ✓ | ✓ | ✓ | ✓ |
| Kitty | ✓ | ✓ | ✓ | ✓ |
| GNOME Terminal | ✓ | ✓ | ✓ | ✓ |
| Windows Terminal | ✓ | ✓ | ✓ | ✓ |
| Termux | ✓ | ✓ | ✓ | ✓ |
| tmux | ✓ | ✓* | ✓ | ✓ |

*Requires `set -g default-terminal "tmux-256color"` and `set -ga terminal-overrides ",*:Tc"`

## Project Structure

```
desktop-tui/
├── CMakeLists.txt
├── src/
│   ├── core/           # Platform-independent utilities
│   │   ├── event.hpp   # Event system
│   │   ├── signal.hpp  # Signal/slot
│   │   ├── rect.hpp    # Rectangle geometry
│   │   ├── colors.hpp  # Color & styling
│   │   └── string_utils.hpp
│   ├── platform/       # Platform abstraction
│   │   ├── terminal.hpp
│   │   ├── terminal_posix.cpp
│   │   ├── terminal_win.cpp
│   │   ├── terminal_android.cpp
│   │   ├── terminal_generic.cpp
│   │   ├── input.hpp
│   │   ├── input_posix.cpp
│   │   ├── input_win.cpp
│   │   ├── input_android.cpp
│   │   └── input_generic.cpp
│   ├── ui/             # UI subsystem
│   │   ├── renderer.hpp
│   │   ├── widget.hpp
│   │   ├── panel.hpp
│   │   ├── label.hpp
│   │   ├── border.hpp
│   │   └── list.hpp
│   ├── desktop/        # Virtual desktop system
│   │   ├── desktop.hpp
│   │   └── desktop_manager.hpp
│   ├── window/         # Window management
│   │   ├── window.hpp
│   │   └── window_manager.hpp
│   └── main.cpp
└── include/
    └── tui.hpp         # Single include entry point
```

## Design Principles

1. **Zero Dependencies**: No ncurses, no external libraries
2. **C++17 Only**: Modern features (std::optional, std::variant, structured bindings)
3. **RAII**: Resource management via smart pointers and destructors
4. **Platform Abstraction**: Clean interface per platform, compiled conditionally
5. **Double-Buffered Rendering**: Front/back buffers with dirty-region optimization
6. **Event-Driven**: Non-blocking input polling, event bus dispatch

## License

MIT
