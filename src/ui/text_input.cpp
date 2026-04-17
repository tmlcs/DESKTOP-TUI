#include "text_input.hpp"
#include "core/string_utils.hpp"
#include <algorithm>
#include <cstring>
#include <mutex>

namespace tui {

// --- Implementación de ClipboardImpl Thread-Safe ---
std::mutex ClipboardImpl::mtx_;
std::string ClipboardImpl::content_;

void ClipboardImpl::set(const std::string& text) {
    std::lock_guard<std::mutex> lock(mtx_);
    content_ = text;
}

std::string ClipboardImpl::get() {
    std::lock_guard<std::mutex> lock(mtx_);
    return content_;
}

// --- Clase Clipboard interna (legacy, usa ClipboardImpl) ---
class Clipboard {
public:
    static void set(const std::string& text) {
        ClipboardImpl::set(text);
    }
    
    static std::string get() {
        return ClipboardImpl::get();
    }
};

// --- Constructor ---

TextInput::TextInput(const std::string& text) 
    : text_(text), cursor_byte_idx_(text.size()) {
    last_blink_time_ = std::chrono::steady_clock::now();
}

// --- Configuración ---

void TextInput::set_placeholder(const std::string& placeholder) {
    placeholder_ = placeholder;
}

void TextInput::set_mask_char(std::optional<char32_t> mask) {
    mask_char_ = mask;
}

void TextInput::set_max_length(size_t max_len) {
    max_length_ = max_len;
}

void TextInput::set_value(const std::string& new_value) {
    text_ = new_value;
    if (cursor_byte_idx_ > text_.size()) {
        cursor_byte_idx_ = text_.size();
    }
    clear_selection();
}

std::string TextInput::selected_text() const {
    if (!has_selection()) return "";
    size_t start = std::min(selection_start_, selection_end_);
    size_t end = std::max(selection_start_, selection_end_);
    return text_.substr(start, end - start);
}

size_t TextInput::get_cursor_byte_index() const {
    return cursor_byte_idx_;
}

// --- Movimiento del Cursor ---

void TextInput::move_cursor_left(bool extend_selection) {
    if (cursor_byte_idx_ == 0) return;
    
    const char* ptr = text_.c_str();
    const char* target = ptr + cursor_byte_idx_;
    
    while (ptr < target) {
        const char* next = ptr;
        utf8_decode(next, text_.c_str() + text_.size());
        if (next >= target) break;
        ptr = next;
    }
    
    cursor_byte_idx_ = static_cast<size_t>(ptr - text_.c_str());
    
    if (!extend_selection) {
        clear_selection();
    } else if (selection_start_ == selection_end_) {
        selection_start_ = cursor_byte_idx_;
        selection_end_ = cursor_byte_idx_;
    } else {
        selection_end_ = cursor_byte_idx_;
    }
}

void TextInput::move_cursor_right(bool extend_selection) {
    if (cursor_byte_idx_ >= text_.size()) return;
    
    const char* ptr = text_.c_str() + cursor_byte_idx_;
    const char* end = text_.c_str() + text_.size();
    
    utf8_decode(ptr, end);
    cursor_byte_idx_ = static_cast<size_t>(ptr - text_.c_str());
    
    if (!extend_selection) {
        clear_selection();
    } else if (selection_start_ == selection_end_) {
        selection_start_ = cursor_byte_idx_;
        selection_end_ = cursor_byte_idx_;
    } else {
        selection_end_ = cursor_byte_idx_;
    }
}

void TextInput::move_cursor_to_home(bool extend_selection) {
    size_t old_pos = cursor_byte_idx_;
    cursor_byte_idx_ = 0;
    
    if (!extend_selection) {
        clear_selection();
    } else if (selection_start_ == selection_end_) {
        selection_start_ = old_pos;
        selection_end_ = 0;
    } else {
        selection_end_ = 0;
    }
}

void TextInput::move_cursor_to_end(bool extend_selection) {
    size_t old_pos = cursor_byte_idx_;
    cursor_byte_idx_ = text_.size();
    
    if (!extend_selection) {
        clear_selection();
    } else if (selection_start_ == selection_end_) {
        selection_start_ = old_pos;
        selection_end_ = text_.size();
    } else {
        selection_end_ = text_.size();
    }
}

void TextInput::clear_selection() {
    selection_start_ = cursor_byte_idx_;
    selection_end_ = cursor_byte_idx_;
}

// --- Edición de Texto ---

void TextInput::insert_char(const std::string& ch) {
    if (max_length_ > 0 && text_.size() + ch.size() > max_length_) {
        return;
    }
    
    if (has_selection()) {
        size_t start = std::min(selection_start_, selection_end_);
        size_t end = std::max(selection_start_, selection_end_);
        text_.erase(start, end - start);
        cursor_byte_idx_ = start;
        clear_selection();
    }
    
    text_.insert(cursor_byte_idx_, ch);
    cursor_byte_idx_ += ch.size();
}

void TextInput::delete_char_backwards() {
    if (has_selection()) {
        size_t start = std::min(selection_start_, selection_end_);
        size_t end = std::max(selection_start_, selection_end_);
        text_.erase(start, end - start);
        cursor_byte_idx_ = start;
        clear_selection();
        return;
    }
    
    if (cursor_byte_idx_ == 0) return;
    
    const char* ptr = text_.c_str();
    const char* target = ptr + cursor_byte_idx_;
    size_t prev_byte_idx = 0;
    
    while (ptr < target) {
        const char* next = ptr;
        utf8_decode(next, text_.c_str() + text_.size());
        if (next >= target) break;
        prev_byte_idx = static_cast<size_t>(ptr - text_.c_str());
        ptr = next;
    }
    
    text_.erase(prev_byte_idx, cursor_byte_idx_ - prev_byte_idx);
    cursor_byte_idx_ = prev_byte_idx;
}

void TextInput::delete_char_forwards() {
    if (has_selection()) {
        size_t start = std::min(selection_start_, selection_end_);
        size_t end = std::max(selection_start_, selection_end_);
        text_.erase(start, end - start);
        cursor_byte_idx_ = start;
        clear_selection();
        return;
    }
    
    if (cursor_byte_idx_ >= text_.size()) return;
    
    const char* ptr = text_.c_str() + cursor_byte_idx_;
    const char* end = text_.c_str() + text_.size();
    
    utf8_decode(ptr, end);
    size_t char_size = static_cast<size_t>(ptr - (text_.c_str() + cursor_byte_idx_));
    
    text_.erase(cursor_byte_idx_, char_size);
}

// --- Portapapeles ---

void TextInput::copy_to_clipboard() {
    if (has_selection()) {
        Clipboard::set(selected_text());
    }
}

void TextInput::cut_to_clipboard() {
    if (has_selection()) {
        copy_to_clipboard();
        size_t start = std::min(selection_start_, selection_end_);
        size_t end = std::max(selection_start_, selection_end_);
        text_.erase(start, end - start);
        cursor_byte_idx_ = start;
        clear_selection();
    }
}

void TextInput::paste_from_clipboard() {
    std::string clipboard = Clipboard::get();
    if (!clipboard.empty()) {
        std::string clean;
        for (char c : clipboard) {
            if (c >= 32 || c == '\t') {
                clean += c;
            }
        }
        insert_char(clean);
    }
}

void TextInput::select_all() {
    selection_start_ = 0;
    selection_end_ = text_.size();
}

int TextInput::get_visual_cursor_column() const {
    std::string substr = text_.substr(0, cursor_byte_idx_);
    return static_cast<int>(display_width(substr));
}

// --- Manejo de Eventos ---

bool TextInput::on_key_press(const Event& e) {
    last_blink_time_ = std::chrono::steady_clock::now();
    cursor_visible_ = true;
    
    bool shift = e.mods.shift;
    bool ctrl = e.mods.control;
    
    using namespace Keys;
    
    if (ctrl) {
        switch (e.key_code) {
            case 'C': case 'c':
                copy_to_clipboard();
                return true;
            case 'X': case 'x':
                cut_to_clipboard();
                return true;
            case 'V': case 'v':
                paste_from_clipboard();
                return true;
            case 'A': case 'a':
                select_all();
                return true;
            default:
                break;
        }
    }
    
    switch (e.key_code) {
        case ArrowLeft:
            move_cursor_left(shift);
            return true;
        case ArrowRight:
            move_cursor_right(shift);
            return true;
        case Home:
            move_cursor_to_home(shift);
            return true;
        case End:
            move_cursor_to_end(shift);
            return true;
        case Backspace:
            delete_char_backwards();
            return true;
        case Delete_:
            delete_char_forwards();
            return true;
        default:
            break;
    }
    
    // Insertar carácter imprimible
    if (e.key_code >= 32 && e.key_code < 127) {
        std::string ch(1, static_cast<char>(e.key_code));
        insert_char(ch);
        return true;
    }
    
    return false;
}

bool TextInput::on_mouse_down(const Event& e) {
    if (e.mouse_button != 0) return false; // Solo click izquierdo
    
    int click_x = e.mouse_x - bounds_.x;
    int click_y = e.mouse_y - bounds_.y;
    
    if (click_x < 0 || click_x >= bounds_.w || click_y < 0 || click_y >= bounds_.h) {
        return false;
    }
    
    is_dragging_ = true;
    clear_selection();
    
    int current_width = 0;
    size_t byte_idx = 0;
    const char* ptr = text_.c_str();
    const char* end = ptr + text_.size();
    
    while (ptr < end) {
        const char* next = ptr;
        utf8_decode(next, end);
        int char_width = mask_char_.has_value() ? 1 : display_width(std::string(ptr, next - ptr));
        
        if (current_width + char_width > click_x + scroll_offset_) {
            break;
        }
        
        current_width += char_width;
        byte_idx = static_cast<size_t>(next - text_.c_str());
        ptr = next;
    }
    
    cursor_byte_idx_ = byte_idx;
    return true;
}

bool TextInput::on_mouse_move(const Event& e) {
    if (!is_dragging_) return false;
    
    int drag_x = e.mouse_x - bounds_.x;
    if (drag_x < 0 || drag_x >= bounds_.w) return false;
    
    return true;
}

bool TextInput::on_mouse_up(const Event& e) {
    (void)e;
    is_dragging_ = false;
    return false;
}

bool TextInput::handle_event(const Event& e) {
    if (!visible_) return false;
    
    switch (e.type) {
        case EventType::KeyPress:
            return on_key_press(e);
        case EventType::MouseDown:
            return on_mouse_down(e);
        case EventType::MouseMove:
            return on_mouse_move(e);
        case EventType::MouseUp:
            return on_mouse_up(e);
        default:
            return false;
    }
}

// --- Renderizado ---

void TextInput::render(Renderer& renderer) {
    if (!visible_) return;
    
    Style bg_style = Style::Default();
    if (focused_) {
        bg_style.bg = Color::RGB(0, 50, 100);
    } else {
        bg_style.bg = Color::RGB(30, 30, 30);
    }
    
    int term_rows = renderer.rows();
    int term_cols = renderer.cols();
    
    for (int y = bounds_.y; y < bounds_.y + bounds_.h && y < term_rows; y++) {
        for (int x = bounds_.x; x < bounds_.x + bounds_.w && x < term_cols; x++) {
            renderer.write(x, y, " ", bg_style);
        }
    }
    
    int width = bounds_.w;
    int height = bounds_.h;
    
    if (height == 0) return;
    
    std::string display_text;
    if (mask_char_.has_value()) {
        const char* ptr = text_.c_str();
        const char* end = ptr + text_.size();
        while (ptr < end) {
            const char* next = ptr;
            utf8_decode(next, end);
            display_text += static_cast<char>(*mask_char_);
            ptr = next;
        }
    } else {
        display_text = text_;
    }
    
    int cursor_col = get_visual_cursor_column();
    if (cursor_col < scroll_offset_) {
        scroll_offset_ = cursor_col;
    } else if (cursor_col >= scroll_offset_ + width) {
        scroll_offset_ = cursor_col - width + 1;
    }
    
    std::string visible_text;
    int current_col = 0;
    const char* ptr = display_text.c_str();
    const char* end = ptr + display_text.size();
    
    while (ptr < end) {
        const char* next = ptr;
        utf8_decode(next, end);
        int char_w = display_width(std::string(ptr, next - ptr));
        
        if (current_col >= scroll_offset_ && current_col < scroll_offset_ + width) {
            visible_text.append(ptr, next - ptr);
        }
        
        current_col += char_w;
        ptr = next;
        
        if (current_col >= scroll_offset_ + width) break;
    }
    
    Style text_style = Style::Default();
    if (focused_) {
        text_style.fg = Color::Pal(15);
        text_style.bold = true;
    } else {
        text_style.fg = Color::Pal(7);
    }
    
    int draw_x = bounds_.x - scroll_offset_;
    renderer.write(draw_x, bounds_.y, visible_text, text_style);
    
    if (text_.empty() && !placeholder_.empty() && !focused_) {
        Style ph_style = Style::Default();
        ph_style.fg = Color::Pal(244);
        renderer.write(bounds_.x, bounds_.y, truncate(placeholder_, width), ph_style);
    }
    
    if (focused_ && cursor_visible_) {
        int cursor_x = bounds_.x + (cursor_col - scroll_offset_);
        if (cursor_x >= bounds_.x && cursor_x < bounds_.x + width) {
            Style cursor_style = Style::Default();
            cursor_style.bg = Color::Pal(15);
            cursor_style.fg = Color::Pal(0);
            cursor_style.reverse = true;
            renderer.write(cursor_x, bounds_.y, " ", cursor_style);
        }
    }
    
    if (focused_) {
        Style border_style = Style::Default();
        border_style.fg = Color::Pal(6);
        renderer.draw_box(bounds_, border_style, Style::Default());
    }
}

void TextInput::update(double delta_time) {
    (void)delta_time;
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(now - last_blink_time_).count();
    
    if (elapsed >= BLINK_INTERVAL_MS) {
        cursor_visible_ = !cursor_visible_;
        last_blink_time_ = now;
    }
}

} // namespace tui
