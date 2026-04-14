cmake_minimum_required(VERSION 3.14)

# Read version from VERSION file
file(STRINGS "${CMAKE_SOURCE_DIR}/VERSION" PROJECT_VERSION LIMIT_COUNT 1)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)

project(desktop-tui LANGUAGES CXX VERSION ${PROJECT_VERSION})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(WIN32)
    add_compile_definitions(TUI_PLATFORM_WINDOWS)
elseif(ANDROID)
    add_compile_definitions(TUI_PLATFORM_ANDROID)
elseif(APPLE)
    add_compile_definitions(TUI_PLATFORM_MACOS)
elseif(UNIX)
    add_compile_definitions(TUI_PLATFORM_LINUX)
endif()

# Core sources (always compiled)
set(CORE_SOURCES
    src/core/event.hpp
    src/core/signal.hpp
    src/core/rect.hpp
    src/core/colors.hpp
    src/core/string_utils.hpp
    src/core/logger.hpp
)

# Platform sources
if(WIN32)
    set(PLATFORM_SOURCES
        src/platform/terminal_win.cpp
        src/platform/input_win.cpp
    )
elseif(UNIX OR APPLE)
    set(PLATFORM_SOURCES
        src/platform/terminal_posix.cpp
        src/platform/input_posix.cpp
    )
elseif(ANDROID)
    set(PLATFORM_SOURCES
        src/platform/terminal_android.cpp
        src/platform/input_android.cpp
    )
else()
    set(PLATFORM_SOURCES
        src/platform/terminal_generic.cpp
        src/platform/input_generic.cpp
    )
endif()

# UI sources
set(UI_SOURCES
    src/ui/renderer.hpp
    src/ui/renderer.cpp
    src/ui/widget.hpp
    src/ui/panel.hpp
    src/ui/panel.cpp
    src/ui/label.hpp
    src/ui/border.hpp
    src/ui/list.hpp
)

# Desktop sources
set(DESKTOP_SOURCES
    src/desktop/desktop.hpp
    src/desktop/desktop.cpp
    src/desktop/desktop_manager.hpp
    src/desktop/desktop_manager.cpp
)

# Window sources
set(WINDOW_SOURCES
    src/window/window.hpp
    src/window/window.cpp
)

# Main
set(MAIN_SOURCES
    src/main.cpp
)

# Executable
add_executable(${PROJECT_NAME}
    ${CORE_SOURCES}
    ${PLATFORM_SOURCES}
    ${UI_SOURCES}
    ${DESKTOP_SOURCES}
    ${WINDOW_SOURCES}
    ${MAIN_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Platform-specific link libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE user32)
endif()

# Install
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# --- Options ---
option(ENABLE_TESTS "Build unit tests" OFF)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

# Sanitizers
if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/include/version.hpp.in
    ${CMAKE_BINARY_DIR}/generated/version.hpp
    @ONLY
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_BINARY_DIR}/generated)

# Unit tests
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
