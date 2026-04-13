#ifndef TUI_CORE_STRING_UTILS_HPP
#define TUI_CORE_STRING_UTILS_HPP

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <codecvt>
#include <locale>

namespace tui {

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
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) start++;
    auto end = s.end();
    do { end--; } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

/// Pad or truncate string to given width
inline std::string pad(const std::string& s, size_t width, char fill = ' ') {
    if (s.size() >= width) return s.substr(0, width);
    return s + std::string(width - s.size(), fill);
}

/// Center text within given width
inline std::string center(const std::string& s, size_t width, char fill = ' ') {
    if (s.size() >= width) return s.substr(0, width);
    size_t padding = width - s.size();
    size_t left = padding / 2;
    size_t right = padding - left;
    return std::string(left, fill) + s + std::string(right, fill);
}

/// Right-align text within given width
inline std::string right_align(const std::string& s, size_t width, char fill = ' ') {
    if (s.size() >= width) return s.substr(0, width);
    return std::string(width - s.size(), fill) + s;
}

/// Repeat a string n times
inline std::string repeat(const std::string& s, size_t n) {
    std::string result;
    for (size_t i = 0; i < n; i++) result += s;
    return result;
}

/// Count visible display width of a UTF-8 string
/// Handles CJK wide characters (which take 2 cells)
inline size_t display_width(const std::string& utf8_str) {
    std::wstring_convert<std::codecvt_utf8_utf16<char32_t>, char32_t> converter;
    auto str = converter.from_bytes(utf8_str);
    size_t width = 0;
    for (char32_t ch : str) {
        // CJK Unified Ideographs and extensions (most common wide ranges)
        if ((ch >= 0x4E00 && ch <= 0x9FFF) ||    // CJK Unified Ideographs
            (ch >= 0x3400 && ch <= 0x4DBF) ||     // CJK Extension A
            (ch >= 0x20000 && ch <= 0x2A6DF) ||   // CJK Extension B
            (ch >= 0x2A700 && ch <= 0x2B73F) ||   // CJK Extension C
            (ch >= 0x2B740 && ch <= 0x2B81F) ||   // CJK Extension D
            (ch >= 0x2B820 && ch <= 0x2CEAF) ||   // CJK Extension E
            (ch >= 0xF900 && ch <= 0xFAFF) ||     // CJK Compatibility Ideographs
            (ch >= 0x2F800 && ch <= 0x2FA1F) ||   // CJK Compatibility Ideographs Supplement
            (ch >= 0xAC00 && ch <= 0xD7AF) ||     // Hangul Syllables
            (ch >= 0x1100 && ch <= 0x115F) ||     // Hangul Jamo
            (ch >= 0x3040 && ch <= 0x309F) ||     // Hiragana
            (ch >= 0x30A0 && ch <= 0x30FF) ||     // Katakana
            (ch >= 0xFF01 && ch <= 0xFF60) ||     // Fullwidth ASCII
            (ch >= 0xFFE0 && ch <= 0xFFE6) ||     // Fullwidth symbols
            (ch >= 0x2E80 && ch <= 0x2EFF) ||     // CJK Radicals
            (ch >= 0x31C0 && ch <= 0x31EF)) {     // CJK Strokes
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

/// Truncate UTF-8 string to display width
inline std::string truncate(const std::string& utf8_str, size_t max_width) {
    std::wstring_convert<std::codecvt_utf8_utf16<char32_t>, char32_t> converter;
    auto str = converter.from_bytes(utf8_str);
    if (str.size() <= max_width) return utf8_str;
    return converter.to_bytes(str.substr(0, max_width));
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

/// Wrap text to multiple lines at given width
inline std::vector<std::string> word_wrap(const std::string& text, size_t width) {
    std::vector<std::string> lines;
    std::string line;
    std::istringstream stream(text);
    std::string word;
    while (stream >> word) {
        if (line.empty()) {
            line = word;
        } else if (line.size() + 1 + word.size() <= width) {
            line += " " + word;
        } else {
            lines.push_back(line);
            line = word;
        }
    }
    if (!line.empty()) lines.push_back(line);
    if (lines.empty()) lines.push_back("");
    return lines;
}

} // namespace tui

#endif // TUI_CORE_STRING_UTILS_HPP
