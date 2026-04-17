# Critical Bug Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix 7 critical bugs identified in code audit, delivered in 3 waves by regression risk.

**Architecture:** Each wave is a self-contained set of changes with tests, building to a passing, mergeable state. Version bumps follow SemVer (PATCH releases).

**Tech Stack:** C++17, CMake, custom test framework.

---

## Wave 1: Safe Fixes (v0.2.1)

### Task 1: Fix `utf8_decode` boundary check + add test

**Files:**
- Modify: `src/core/string_utils.hpp:26`
- Modify: `tests/test_critical_fixes.cpp`

- [ ] **Step 1: Add test for truncated UTF-8**

In `tests/test_critical_fixes.cpp`, add this function before `run_critical_fixes_main()`:

```cpp
void test_c5_utf8_decode_truncated() {
    printf("\n--- C5: utf8_decode boundary safety ---\n");

    TEST("truncated 3-byte UTF-8 returns 0",
        []() {
            const char buf[] = "\xE6\x97";
            const char* p = buf;
            const char* end = buf + 2;
            char32_t result = tui::utf8_decode(p, end);
            return result == 0;
        }());

    TEST("truncated 2-byte UTF-8 returns 0",
        []() {
            const char buf[] = "\xC2";
            const char* p = buf;
            const char* end = buf + 1;
            char32_t result = tui::utf8_decode(p, end);
            return result == 0;
        }());

    TEST("valid 3-byte UTF-8 decodes correctly",
        []() {
            const char buf[] = "\xE6\x97\xA5";
            const char* p = buf;
            const char* end = buf + 3;
            char32_t result = tui::utf8_decode(p, end);
            return result == 0x65E5;
        }());

    TEST("decode at exact boundary returns 0",
        []() {
            const char buf[] = "";
            const char* p = buf;
            const char* end = buf;
            char32_t result = tui::utf8_decode(p, end);
            return result == 0;
        }());
}
```

Add call in `run_critical_fixes_main()`:
```cpp
test_c5_utf8_decode_truncated();
```

- [ ] **Step 2: Run test to verify current behavior**

```bash
cd /var/tmlcs/workspace/desktop-tui/build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTS=ON
make -j$(nproc)
./tests/test_all
```

Expected: Tests pass (current code happens to work by reading \0), but the boundary check is still wrong semantically.

- [ ] **Step 3: Fix the boundary check**

In `src/core/string_utils.hpp`, line 26:
```cpp
// BEFORE:
if (p > end) return 0;
// AFTER:
if (p >= end) return 0;
```

- [ ] **Step 4: Run test to verify it passes**

```bash
cd /var/tmlcs/workspace/desktop-tui/build
make -j$(nproc)
./tests/test_all
```

Expected: All tests pass, 0 failures.

- [ ] **Step 5: Commit**

```bash
git add src/core/string_utils.hpp tests/test_critical_fixes.cpp
git commit -m "fix: utf8_decode boundary check (C5)

Change p > end to p >= end to prevent reading past buffer
when UTF-8 sequences are truncated at exact boundary."
```

---

### Task 2: Fix Windows `SetConsoleMode` and `flush()`

**Files:**
- Modify: `src/platform/terminal_win.cpp`

- [ ] **Step 1: Fix `SetConsoleMode` double call**

In `src/platform/terminal_win.cpp`, replace the entire `enter_raw_mode()` method:

```cpp
void enter_raw_mode() override {
    if (raw_mode_) return;
    DWORD mode = 0;
    GetConsoleMode(hIn_, &mode);
    orig_input_mode_ = mode;
    SetConsoleMode(hIn_,
        (mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT))
        | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    GetConsoleMode(hOut_, &mode);
    orig_output_mode_ = mode;
    if (supports_vt_) {
        SetConsoleMode(hOut_, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
    }

    if (supports_vt_) write("\033[?1006;1000;1015;2004h");
    raw_mode_ = true;
}
```

- [ ] **Step 2: Fix `flush()` no-op**

```cpp
// BEFORE:
void flush() override { /* stdout is unbuffered in raw mode */ }
// AFTER:
void flush() override { fflush(stdout); }
```

- [ ] **Step 3: Commit**

```bash
git add src/platform/terminal_win.cpp
git commit -m "fix: Windows input and flush (C6, C7)

C6: Combine SetConsoleMode calls to prevent raw mode being overwritten.
C7: Add fflush(stdout) to flush() for non-VT mode."
```

---

### Task 3: VERSION + CHANGELOG bump to 0.2.1

- [ ] **Step 1: Update VERSION**

Change `0.2.0` to `0.2.1`.

- [ ] **Step 2: Update CHANGELOG.md**

Add before `[0.2.0]`:

```markdown
## [0.2.1] - 2026-04-15

### Fixed

- **utf8_decode boundary check (C5)**: Changed `p > end` to `p >= end` in `utf8_decode()` to prevent reading past buffer when UTF-8 sequences are truncated at exact byte boundary. Added unit tests for truncated 2-byte and 3-byte sequences.
- **Windows SetConsoleMode double call (C6)**: Combined two `SetConsoleMode()` calls in `enter_raw_mode()` into a single call with unified flags, preventing the second call from overwriting the raw mode configuration.
- **Windows flush no-op (C7)**: Added `fflush(stdout)` to `WindowsTerminal::flush()` to ensure output appears on screen in non-VT mode.
```

- [ ] **Step 3: Commit**

```bash
git add VERSION CHANGELOG.md
git commit -m "chore: bump version to 0.2.1

Release: fix C5 (utf8_decode boundary), C6 (Windows SetConsoleMode),
C7 (Windows flush)."
```

### Wave 1: Verification

```bash
cd /var/tmlcs/workspace/desktop-tui/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
ctest --output-on-failure
./desktop-tui --version
```

---

## Wave 2: Medium Risk (v0.2.2)

### Task 4: Extract `is_wide_codepoint()` shared function

**Files:**
- Modify: `src/core/string_utils.hpp`

- [ ] **Step 1: Add `is_wide_codepoint()` function**

After `utf8_decode()` and before `display_width()`:

```cpp
inline bool is_wide_codepoint(char32_t ch) {
    return (ch >= 0x4E00 && ch <= 0x9FFF) ||
           (ch >= 0x3400 && ch <= 0x4DBF) ||
           (ch >= 0x20000 && ch <= 0x2A6DF) ||
           (ch >= 0x2A700 && ch <= 0x2B73F) ||
           (ch >= 0x2B740 && ch <= 0x2B81F) ||
           (ch >= 0x2B820 && ch <= 0x2CEAF) ||
           (ch >= 0xF900 && ch <= 0xFAFF) ||
           (ch >= 0x2F800 && ch <= 0x2FA1F) ||
           (ch >= 0xAC00 && ch <= 0xD7AF) ||
           (ch >= 0x1100 && ch <= 0x115F) ||
           (ch >= 0x3040 && ch <= 0x309F) ||
           (ch >= 0x30A0 && ch <= 0x30FF) ||
           (ch >= 0xFF01 && ch <= 0xFF60) ||
           (ch >= 0xFFE0 && ch <= 0xFFE6) ||
           (ch >= 0x2E80 && ch <= 0x2EFF) ||
           (ch >= 0x31C0 && ch <= 0x31EF);
}
```

- [ ] **Step 2: Refactor `display_width()` to use it**

Replace the large if-block with: `if (is_wide_codepoint(ch)) {`

- [ ] **Step 3: Refactor `truncate()` to use it**

Replace the large if-block with: `if (is_wide_codepoint(ch)) {`

- [ ] **Step 4: Commit**

```bash
git add src/core/string_utils.hpp
git commit -m "refactor: extract is_wide_codepoint() shared function (S4)

Eliminates inconsistency between display_width() and truncate()
by using a single source of truth for wide character ranges."
```

---

### Task 5: Fix renderer column tracking for wide characters (C1)

**Files:**
- Modify: `src/ui/renderer.hpp`
- Modify: `tests/test_critical_fixes.cpp`

- [ ] **Step 1: Fix the `write()` method**

Replace the entire `write()` method body:

```cpp
void write(int x, int y, const std::string& text, const Style& style) {
    if (y >= rows_ || y < -1000) return;
    int col = x;
    int row = y;

    const char* p = text.data();
    const char* end = p + text.size();
    while (p < end) {
        char32_t ch = utf8_decode(p, end);
        if (ch == 0 && p >= end) break;

        if (ch == '\n') {
            col = x;
            row++;
            continue;
        }

        int cell_width = 0;
        if (ch < 0x20 || ch == 0x7F) {
            cell_width = 0;
        } else if (is_wide_codepoint(ch)) {
            cell_width = 2;
        } else {
            cell_width = 1;
        }

        if (col >= 0 && row >= 0 && col < cols_ && row < rows_) {
            Cell& cell = back_buffer_[row * cols_ + col];
            cell.ch = ch;
            cell.style = style;
            dirty_ = true;
            if (cell_width == 2 && col + 1 < cols_) {
                Cell& cell2 = back_buffer_[row * cols_ + col + 1];
                cell2.ch = ' ';
                cell2.style = style;
            }
        }

        col += cell_width;

        if (col >= cols_) {
            col = x;
            row++;
        }
    }
}
```

- [ ] **Step 2: Add test for CJK column tracking**

```cpp
void test_c1_cjk_column_tracking() {
    printf("\n--- C1: Renderer CJK column tracking ---\n");

    TestTerminal term(80, 24);
    tui::Renderer r(term);
    r.resize(80, 24);
    r.clear();

    r.write(0, 0, "日本", tui::Style::Default());
    r.flush();
    auto output = term.drain_output();

    TEST("CJK text renders without overlap markers",
        output.find("\xE6\x97\xA5") != std::string::npos &&
        output.find("\xE6\x9C\xAC") != std::string::npos);
}
```

- [ ] **Step 3: Verify, commit**

```bash
git add src/ui/renderer.hpp tests/test_critical_fixes.cpp
git commit -m "fix: renderer column tracking for wide characters (C1)

CJK characters now advance the render column by their display
width (2) instead of always incrementing by 1, preventing visual
overlap corruption."
```

---

### Task 6: Fix `Desktop::windows()` to return const reference (S1)

**Files:**
- Modify: `src/desktop/desktop.hpp:64-67`

- [ ] **Step 1: Change return type**

```cpp
const std::vector<std::shared_ptr<Window>>& windows() {
    cleanup_stale_windows();
    return windows_;
}
```

- [ ] **Step 2: Commit**

```bash
git add src/desktop/desktop.hpp
git commit -m "perf: return windows() by const reference (S1)

Eliminates per-frame vector copy in render and input dispatch paths."
```

---

### Task 7: Wave 2 version bump

- [ ] **Step 1:** VERSION `0.2.1` → `0.2.2`
- [ ] **Step 2:** Add `[0.2.2]` to CHANGELOG
- [ ] **Step 3:** Commit

---

## Wave 3: High Risk (v0.2.3)

### Task 8: Fix Panel coordinate system (C2)

**Files:**
- Modify: `src/ui/panel.hpp` (render + handle_event)
- Modify: `tests/test_critical_fixes.cpp`

- [ ] **Step 1: Fix render()**

Convert child bounds to absolute:

```cpp
Rect inner = content_area();
for (auto& child : children_) {
    if (!child->visible()) continue;
    Rect cb = child->bounds();
    Rect abs_cb = {cb.x + inner.x, cb.y + inner.y, cb.w, cb.h};
    auto clipped = inner.intersection(abs_cb);
    if (!clipped.has_value() || clipped->empty()) continue;
    child->set_bounds(abs_cb);
    child->render(r);
    child->set_bounds(cb);
}
```

- [ ] **Step 2: Fix handle_event()**

```cpp
Rect inner = content_area();
for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    auto& child = *it;
    if (child->visible() && child->can_focus()) {
        Rect cb = child->bounds();
        Rect abs_cb = {cb.x + inner.x, cb.y + inner.y, cb.w, cb.h};
        if (abs_cb.contains(e.mouse_x, e.mouse_y)) {
            ...
```

- [ ] **Step 3: Add tests, commit**

---

### Task 9: Fix SIGWINCH handler (C3) + signal safety (C4)

**Files:**
- Modify: `src/platform/terminal_posix.cpp`
- Modify: `src/platform/terminal.hpp`
- Modify: `src/main.cpp`

- [ ] **Step 1: Save/restore SIGWINCH handler**
- [ ] **Step 2: Replace direct pointer with volatile flag**
- [ ] **Step 3: Update main.cpp to poll flag**
- [ ] **Step 4: Commit**

---

### Task 10: Wave 3 version bump

- [ ] VERSION `0.2.2` → `0.2.3`, CHANGELOG, commit.
