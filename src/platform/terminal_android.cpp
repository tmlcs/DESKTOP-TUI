// Android (Termux) terminal implementation
// Essentially same as POSIX but with Termux-specific adjustments

#include "terminal.hpp"

#ifdef TUI_PLATFORM_ANDROID

// Android/Termux uses the same POSIX APIs as Linux
// Include the POSIX implementation
#include "terminal_posix.cpp"

#endif // TUI_PLATFORM_ANDROID
