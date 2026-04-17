#ifndef TUI_CORE_CONFIG_HPP
#define TUI_CORE_CONFIG_HPP

#include <cstddef>
#include <chrono>

namespace tui {

/// Configuration constants for the TUI framework
/// Centralized configuration to avoid magic numbers
struct Config {
    // ========================================================================
    // Input Configuration
    // ========================================================================
    
    /// Maximum input buffer size to prevent DoS attacks
    /// 64KB limit prevents memory exhaustion from malicious input
    static constexpr size_t MAX_INPUT_BUFFER_SIZE = 64 * 1024; // 64KB
    
    /// Initial input buffer reservation (optimization)
    static constexpr size_t INPUT_BUFFER_INITIAL_RESERVE = 1024; // 1KB
    
    // ========================================================================
    // Event Loop Configuration
    // ========================================================================
    
    /// Sleep duration when idle (no input/events)
    /// 50ms gives ~20fps wake-up rate for responsive input checking
    /// while minimizing CPU usage
    static constexpr std::chrono::milliseconds IDLE_SLEEP_DURATION{50};
    
    // ========================================================================
    // Rendering Configuration
    // ========================================================================
    
    /// Maximum terminal dimensions (safety limit)
    static constexpr int MAX_TERMINAL_COLS = 1024;
    static constexpr int MAX_TERMINAL_ROWS = 1024;
    
    /// Minimum terminal dimensions
    static constexpr int MIN_TERMINAL_COLS = 40;
    static constexpr int MIN_TERMINAL_ROWS = 10;
    
    // ========================================================================
    // Desktop Management Configuration
    // ========================================================================
    
    /// Default number of desktops on startup
    static constexpr int DEFAULT_DESKTOP_COUNT = 3;
    
    /// Maximum number of desktops allowed
    static constexpr int MAX_DESKTOP_COUNT = 20;
    
    // ========================================================================
    // Window Management Configuration
    // ========================================================================
    
    /// Minimum window size
    static constexpr int MIN_WINDOW_WIDTH = 20;
    static constexpr int MIN_WINDOW_HEIGHT = 5;
    
    // ========================================================================
    // Security Configuration
    // ========================================================================
    
    /// Maximum title length to prevent injection attacks
    static constexpr size_t MAX_TITLE_LENGTH = 256;
    
    /// Enable bracketed paste mode (security feature)
    static constexpr bool ENABLE_BRACKETED_PASTE = true;
    
    // ========================================================================
    // Performance Configuration
    // ========================================================================
    
    /// Enable dirty region optimization in renderer
    static constexpr bool ENABLE_DIRTY_REGION_OPTIMIZATION = true;
    
    /// Enable double buffering
    static constexpr bool ENABLE_DOUBLE_BUFFERING = true;
};

} // namespace tui

#endif // TUI_CORE_CONFIG_HPP
