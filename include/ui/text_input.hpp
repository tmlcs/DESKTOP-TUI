#pragma once

#include "widget.hpp"
#include "ui/renderer.hpp"
#include <string>
#include <optional>
#include <chrono>
#include <mutex>

namespace tui {

// Clipboard implementation - thread-safe internal class
class ClipboardImpl {
public:
    static void set(const std::string& text);
    static std::string get();
    
private:
    static std::mutex mtx_;
    static std::string content_;
};

/**
 * @brief Widget de entrada de texto de una sola línea con soporte UTF-8.
 * 
 * Características:
 * - Navegación con flechas, Home, End.
 * - Edición: Insert, Backspace, Delete.
 * - Portapapeles: Ctrl+C (Copiar), Ctrl+X (Cortar), Ctrl+V (Pegar).
 * - Selección de texto con Shift+Flechas o Mouse Drag.
 * - Scroll horizontal automático.
 * - Cursor parpadeante.
 */
class TextInput : public Widget {
public:
    explicit TextInput(const std::string& text = "");
    ~TextInput() override = default;

    // --- Configuración ---
    void set_placeholder(const std::string& placeholder);
    void set_mask_char(std::optional<char32_t> mask); // Para contraseñas
    void set_max_length(size_t max_len);

    // --- Estado ---
    const std::string& value() const { return text_; }
    void set_value(const std::string& new_value);
    bool has_selection() const { return selection_start_ != selection_end_; }
    std::string selected_text() const;

    // Override de Widget
    void render(Renderer& renderer) override;
    bool handle_event(const Event& e) override;
    
    // Actualización interna (no override, método propio)
    void update(double delta_time); // Para el parpadeo del cursor

private:
    // --- Lógica Interna ---
    size_t get_cursor_byte_index() const;
    void move_cursor_left(bool extend_selection);
    void move_cursor_right(bool extend_selection);
    void move_cursor_to_home(bool extend_selection);
    void move_cursor_to_end(bool extend_selection);
    void insert_char(const std::string& ch);
    void delete_char_backwards();
    void delete_char_forwards();
    
    // Métodos públicos para tests (movidos aquí para mantener encapsulamiento)
public:
    void copy_to_clipboard();     // Público para tests
    void cut_to_clipboard();      // Público para tests
    void paste_from_clipboard();  // Público para tests
    void select_all();            // Público para tests
private:
    void clear_selection();
    
    // Calcula la columna visual donde está el cursor (considerando scroll)
    int get_visual_cursor_column() const;
    
    // Manejo de eventos específicos
    bool on_key_press(const Event& e);
    bool on_mouse_down(const Event& e);
    bool on_mouse_move(const Event& e);
    bool on_mouse_up(const Event& e);

    // --- Datos ---
    std::string text_;
    std::string placeholder_;
    std::optional<char32_t> mask_char_; // Si tiene valor, enmascara el texto (ej: '*')
    size_t max_length_ = 0; // 0 = ilimitado

    // Cursor y Selección (índices en bytes para preservar UTF-8)
    size_t cursor_byte_idx_ = 0;
    size_t selection_start_ = 0; // Byte index
    size_t selection_end_ = 0;   // Byte index
    
    // Scroll
    int scroll_offset_ = 0; // Desplazamiento en columnas visuales

    // Estado visual
    bool cursor_visible_ = true;
    std::chrono::steady_clock::time_point last_blink_time_;
    static constexpr double BLINK_INTERVAL_MS = 500.0;

    // Drag de mouse
    bool is_dragging_ = false;
};

} // namespace tui
