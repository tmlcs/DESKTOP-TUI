#ifndef TUI_VERSION_HPP
#define TUI_VERSION_HPP

#define TUI_VERSION_MAJOR 0
#define TUI_VERSION_MINOR 3
#define TUI_VERSION_PATCH 0
#define TUI_VERSION "0.3.0"

namespace tui {
    inline const char* version() { return TUI_VERSION; }
}

#endif // TUI_VERSION_HPP
