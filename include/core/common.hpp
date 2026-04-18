#ifndef TUI_CORE_COMMON_HPP
#define TUI_CORE_COMMON_HPP

/// @file common.hpp
/// @brief Common core types and utilities for the TUI framework
/// 
/// This header consolidates frequently-used core types to reduce include duplication.
/// Include this single header instead of multiple individual core headers.
///
/// Includes:
/// - Point, Rect (geometry types)
/// - Color, Style, Styles (color and styling)
/// - Event, EventType, KeyMods, Keys, EventBus (event system)
/// - UTF-8 utilities: utf8_decode, display_width, truncate, etc.
/// - String utilities: split, trim, pad, center, join, word_wrap, etc.

#include "rect.hpp"
#include "colors.hpp"
#include "event.hpp"
#include "string_utils.hpp"

#endif // TUI_CORE_COMMON_HPP
