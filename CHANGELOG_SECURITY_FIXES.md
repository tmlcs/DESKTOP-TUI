# Security Fixes - Desktop TUI v0.2.6

## Overview
This release addresses three critical security vulnerabilities identified in the security audit of v0.2.5.

## Security Fixes

### SEC-01: Bracketed Paste Injection Prevention
**Severity:** HIGH  
**Files Modified:** `src/platform/input_posix.cpp`

**Issue:**  
Bracketed paste mode (enabled by default) could allow malicious applications to inject arbitrary escape sequences through clipboard content, potentially leading to terminal hijacking or data exfiltration.

**Fix:**  
- Implemented proper parsing of bracketed paste sequences (`ESC[200~` ... `ESC[201~`)
- All content between paste markers is treated as raw text data, not commands
- Paste content is delivered via a `Custom` event type for safe handling by applications
- Added `in_bracketed_paste_` state tracking to prevent sequence interpretation during paste

**Testing:**  
Verified that pasted content containing escape sequences is safely handled as text data.

---

### SEC-02: Title Escape Sequence Injection Prevention
**Severity:** MEDIUM  
**Files Modified:** 
- `src/platform/terminal_posix.cpp`
- `src/platform/terminal_win.cpp`
- `src/platform/terminal_generic.cpp`

**Issue:**  
Window titles set via `set_title()` were not sanitized, allowing attackers to inject OSC (Operating System Command) escape sequences that could:
- Change terminal colors permanently
- Execute terminal-specific commands
- Corrupt the terminal state

**Fix:**  
- Added comprehensive title sanitization in all platform implementations
- Removed dangerous characters: ESC (0x1B), BEL (0x07), CR (0x0D), and control characters (< 0x20 except TAB)
- ESC characters are silently dropped (along with any following sequence)
- BEL and CR are replaced with spaces to preserve title readability
- Other control characters are silently dropped

**Testing:**  
Verified that titles like `"Test\x1b[31mInjected\x07Title"` are sanitized to `"TestInjectedTitle"`.

---

### SEC-03: Dangling Focus After Widget Removal
**Severity:** MEDIUM  
**Files Modified:** `src/ui/panel.hpp`

**Issue:**  
When a focused widget was removed from a panel (via `remove_child()` or `clear_children()`), the focus state was not cleared, potentially leading to:
- Use-after-free if the widget was accessed through stale focus pointers
- Undefined behavior when events were sent to destroyed widgets
- Crash scenarios in complex UI hierarchies

**Fix:**  
- `remove_child()`: Now calls `blur()` on the child before removal if it has focus
- `clear_children()`: Iterates through all children and calls `blur()` on any focused widgets before clearing the container
- Prevents dangling focus references and ensures clean state after widget removal

**Testing:**  
Verified that removing focused widgets does not leave stale focus references.

---

## Verification

All fixes have been tested with:
- Unit tests for sanitization logic
- Integration tests with existing test suite (56 tests passing)
- Manual verification of security properties

Run the verification test:
```bash
g++ -std=c++17 -I src -o test_security test_security.cpp [object files]
./test_security
```

## Compatibility

These changes are **backwards compatible**:
- No API changes
- Existing code continues to work without modification
- Behavior changes only affect edge cases (malicious input)

## Recommendations

Users should upgrade to this version immediately if:
- The application handles untrusted input
- The application displays user-provided window titles
- The application dynamically adds/removes widgets

## Credits

Security issues identified during the comprehensive code audit of Desktop TUI v0.2.5.
