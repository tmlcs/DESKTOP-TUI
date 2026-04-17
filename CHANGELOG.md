# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.8] - 2026-04-17

### Fixed
- Fixed test assertion in `test_rect_safety.cpp` for negative coordinates intersection test (w=15, h=15 instead of w=5, h=5)
- All 56+ tests now passing successfully

### Changed
- Bumped version from 0.2.7 to 0.2.8 (patch release)

---

## [0.2.7] - Previous Release

### Added
- Core TUI framework with event system, signal/slot, rect geometry
- Multi-platform support (POSIX, Windows, Android)
- UI widgets: panel, label, list, text_input
- Desktop manager with virtual desktop switching
- Window system with z-order and focus management
- Thread-safe clipboard implementation
- UTF-8 support with CJK character width handling
- Security features: bracketed paste prevention, title sanitization

### Fixed
- Bounds checking in renderer
- Signal iterator safety during emit
- Panel child clipping to content area
- Desktop manager index and pointer safety
- UTF-8 decoding boundary safety
