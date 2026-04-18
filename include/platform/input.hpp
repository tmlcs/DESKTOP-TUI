#ifndef TUI_PLATFORM_INPUT_HPP
#define TUI_PLATFORM_INPUT_HPP

#include "core/common.hpp"
#include <memory>
#include <optional>

namespace tui {

/// Abstract input handler interface
class IInput {
public:
    virtual ~IInput() = default;

    // Initialize input subsystem
    virtual bool init() = 0;
    virtual void shutdown() = 0;

    // Poll for next event (non-blocking, returns nullopt if no event)
    virtual std::optional<Event> poll() = 0;

    // Check if input is available
    virtual bool has_input() = 0;

    // Get current mouse position
    virtual int mouse_x() const = 0;
    virtual int mouse_y() const = 0;
};

/// Factory: create the appropriate input handler for this platform
std::unique_ptr<IInput> create_input();

} // namespace tui

#endif // TUI_PLATFORM_INPUT_HPP
