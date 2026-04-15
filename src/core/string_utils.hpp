#ifndef TUI_CORE_STRING_UTILS_HPP
#define TUI_CORE_STRING_UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdint>

namespace tui {

/// Decode a single UTF-8 codepoint from the given pointer.
/// Advances `p` past the consumed bytes. Returns 0 on invalid input.
inline char32_t utf8_decode(const char*& p, const char* end) {
    if (p >= end) return 0;
    unsigned char b0 = static_cast<unsigned char>(*p);
    if (b0 < 0x80) { p++; return b0; }
    if (b0 < 0xC0) { p++; return 0; } // continuation byte alone
    int extra = 0;
    if (b0 < 0xE0) { extra = 1; b0 &= 0x1F; }
    else if (b0 < 0xF0) { extra = 2; b0 &= 0x0F; }
    else { extra = 3; b0 &= 0x07; }
    char32_t cp = b0;
    for (int i = 0; i < extra; i++) {
        p++;
        if (p >= end) return 0;
        unsigned char b = static_cast<unsigned char>(*p);
        if ((b & 0xC0) != 0x80) return 0;
        cp = (cp << 6) | (b & 0x3F);
    }
    p++;
    return cp;
}

/// Check if a codepoint has display width 2 (CJK, Hangul, etc.)
inline bool is_wide_codepoint(char32_t ch) {
    return (ch >= 0x4E00 && ch <= 0x9FFF) ||    // CJK Unified Ideographs
           (ch >= 0x3400 && ch <= 0x4DBF) ||     // CJK Extension A
           (ch >= 0x20000 && ch <= 0x2A6DF) ||   // CJK Extension B
           (ch >= 0x2A700 && ch <= 0x2B73F) ||   // CJK Extension C
           (ch >= 0x2B740 && ch <= 0x2B81F) ||   // CJK Extension D
           (ch >= 0x2B820 && ch <= 0x2CEAF) ||   // CJK Extension E
           (ch >= 0xF900 && ch <= 0xFAFF) ||     // CJK Compatibility Ideographs
           (ch >= 0x2F800 && ch <= 0x2FA1F) ||   // CJK Compatibility Supplement
           (ch >= 0xAC00 && ch <= 0xD7AF) ||     // Hangul Syllables
           (ch >= 0x1100 && ch <= 0x115F) ||     // Hangul Jamo
           (ch >= 0x3040 && ch <= 0x309F) ||     // Hiragana
           (ch >= 0x30A0 && ch <= 0x30FF) ||     // Katakana
           (ch >= 0xFF01 && ch <= 0xFF60) ||     // Fullwidth ASCII
           (ch >= 0xFFE0 && ch <= 0xFFE6) ||     // Fullwidth symbols
           (ch >= 0x2E80 && ch <= 0x2EFF) ||     // CJK Radicals
           (ch >= 0x31C0 && ch <= 0x31EF);       // CJK Strokes
}

/// Count visible display width of a UTF-8 string
/// Handles CJK wide characters (which take 2 cells)
inline size_t display_width(const std::string& utf8_str) {
    const char* p = utf8_str.data();
    const char* end = p + utf8_str.size();
    size_t width = 0;
    while (p < end) {
        char32_t ch = utf8_decode(p, end);
        if (ch == 0 && p >= end) break;
        if (is_wide_codepoint(ch)) {
            width += 2;
        } else if (ch < 0x20 || ch == 0x7F) {
            // Control characters: 0 width
            continue;
        } else {
            width += 1;
        }
    }
    return width;
}

/// Truncate UTF-8 string to display width (UTF-8 aware)
inline std::string truncate(const std::string& utf8_str, size_t max_width) {
    size_t dw = display_width(utf8_str);
    if (dw <= max_width) return utf8_str;

    const char* p = utf8_str.data();
    const char* end = p + utf8_str.size();
    size_t width = 0;
    const char* cut = p;
    while (p < end) {
        char32_t ch = utf8_decode(p, end);
        if (ch == 0 && p >= end) break;
        int ch_width = 0;
        if (is_wide_codepoint(ch)) {
            ch_width = 2;
        } else if (ch < 0x20 || ch == 0x7F) {
            ch_width = 0;
        } else {
            ch_width = 1;
        }
        if (width + ch_width > max_width) break;
        width += ch_width;
        cut = p;
    }
    return utf8_str.substr(0, static_cast<size_t>(cut - utf8_str.data()));
}

/// Split a string by delimiter
inline std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

/// Trim whitespace from both ends
inline std::string trim(const std::string& s) {
    if (s.empty()) return s;
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start))) start++;
    if (start == s.end()) return "";
    auto end = s.end();
    do { end--; } while (start != end && std::isspace(static_cast<unsigned char>(*end)));
    return std::string(start, end + 1);
}

/// Pad or truncate string to given display width
inline std::string pad(const std::string& s, size_t width, char fill = ' ') {
    size_t dw = display_width(s);
    if (dw >= width) return truncate(s, width);
    return s + std::string(width - dw, fill);
}

/// Center text within given display width
inline std::string center(const std::string& s, size_t width, char fill = ' ') {
    size_t dw = display_width(s);
    if (dw >= width) return truncate(s, width);
    size_t padding = width - dw;
    size_t left = padding / 2;
    size_t right = padding - left;
    return std::string(left, fill) + s + std::string(right, fill);
}

/// Right-align text within given display width
inline std::string right_align(const std::string& s, size_t width, char fill = ' ') {
    size_t dw = display_width(s);
    if (dw >= width) return truncate(s, width);
    return std::string(width - dw, fill) + s;
}

/// Repeat a string n times
inline std::string repeat(const std::string& s, size_t n) {
    std::string result;
    result.reserve(s.size() * n);
    for (size_t i = 0; i < n; i++) result += s;
    return result;
}

/// Replace all occurrences of 'from' with 'to'
inline std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

/// Join vector of strings with separator
inline std::string join(const std::vector<std::string>& parts, const std::string& sep) {
    if (parts.empty()) return "";
    std::string result = parts[0];
    for (size_t i = 1; i < parts.size(); i++) {
        result += sep + parts[i];
    }
    return result;
}

/// Convert integer to string (shorthand)
template<typename T>
std::string to_str(T val) {
    return std::to_string(val);
}

/// Wrap text to multiple lines at given display width
inline std::vector<std::string> word_wrap(const std::string& text, size_t width) {
    std::vector<std::string> lines;
    std::string line;
    size_t line_dw = 0;  // line display width

    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        size_t word_dw = display_width(word);
        size_t sep_dw = line.empty() ? 0 : 1;

        if (line_dw + sep_dw + word_dw <= width) {
            if (!line.empty()) {
                line += " ";
                line_dw += sep_dw;
            }
            line += word;
            line_dw += word_dw;
        } else {
            lines.push_back(line);
            line = word;
            line_dw = word_dw;
        }
    }
    if (!line.empty()) lines.push_back(line);
    if (lines.empty()) lines.push_back("");
    return lines;
}

} // namespace tui

#endif // TUI_CORE_STRING_UTILS_HPP
