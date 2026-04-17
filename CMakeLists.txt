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

# Core sources (always compiled) - headers only, included from include/
set(CORE_SOURCES
    include/core/event.hpp
    include/core/signal.hpp
    include/core/rect.hpp
    include/core/colors.hpp
    include/core/string_utils.hpp
    include/core/logger.hpp
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
    include/ui/renderer.hpp
    src/ui/renderer.cpp
    include/ui/widget.hpp
    include/ui/panel.hpp
    src/ui/panel.cpp
    include/ui/label.hpp
    include/ui/border.hpp
    include/ui/list.hpp
    include/ui/text_input.hpp
    src/ui/text_input.cpp
)

# Desktop sources
set(DESKTOP_SOURCES
    include/desktop/desktop.hpp
    src/desktop/desktop.cpp
    include/desktop/desktop_manager.hpp
    src/desktop/desktop_manager.cpp
)

# Window sources
set(WINDOW_SOURCES
    include/window/window.hpp
    src/window/window.cpp
)

# Plugin sources (optional)
option(ENABLE_PLUGINS "Enable plugin system" ON)
if(ENABLE_PLUGINS)
    set(PLUGIN_SOURCES
        include/plugins/plugin_interface.hpp
        src/plugins/dynamic_plugin.cpp
        include/plugins/plugin_manager.hpp
        src/plugins/plugin_manager.cpp
    )
    add_compile_definitions(TUI_PLUGINS_ENABLED)
endif()

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
    ${PLUGIN_SOURCES}
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

# Plugin system requires dl on Unix-like systems
if(ENABLE_PLUGINS AND (UNIX OR APPLE))
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
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

# Plugin examples (when plugins are enabled)
if(ENABLE_PLUGINS)
    # Example plugin: Hello World
    add_library(hello_world_plugin SHARED
        examples/plugins/hello_world_plugin.cpp
    )
    target_include_directories(hello_world_plugin PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
    )
    set_target_properties(hello_world_plugin PROPERTIES
        PREFIX "lib"
        OUTPUT_NAME "hello_world"
    )
    
    # Plugin demo application
    add_executable(plugin_demo
        examples/plugins/demo_plugins.cpp
        ${PLUGIN_SOURCES}
    )
    target_include_directories(plugin_demo PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(plugin_demo PRIVATE dl)
endif()

# Unit tests
if(ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
