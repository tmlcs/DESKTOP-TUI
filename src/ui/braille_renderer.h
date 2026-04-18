#pragma once

#include "core/colors.hpp"
#include <vector>
#include <string>

namespace tui {

/**
 * @brief Renderizador de gráficos de alta densidad usando caracteres Braille Unicode.
 * 
 * Los caracteres Braille (U+2800 a U+28FF) permiten representar una matriz de 2x4 píxeles
 * en un solo carácter, logrando una resolución efectiva 8 veces mayor que el renderizado
 * de texto estándar en términos de densidad de píxeles por celda.
 * 
 * Mapeo de bits estándar (ISO/IEC 15851):
 *   ⠿ (0x3F) = ●●  (Bits: 1 8)
 *            ●●  (Bits: 2 9)
 *            ●●  (Bits: 3 10)
 *            ●●  (Bits: 4 11) -> En realidad es 2x4:
 * 
 * Layout de puntos Braille:
 *   1 4
 *   2 5
 *   3 6
 *   7 8
 * 
 * Valor = 1*(p1) + 2*(p2) + 4*(p3) + 8*(p4) + 16*(p5) + 32*(p6) + 64*(p7) + 128*(p8) + 0x2800
 */
class BrailleRenderer {
public:
    // Configuración de la cuadrícula Braille
    static constexpr int BRAILLE_WIDTH = 2;
    static constexpr int BRAILLE_HEIGHT = 4;
    static constexpr uint32_t BRAILLE_BASE = 0x2800;

    // Matriz de pesos para cada posición (x, y) en la celda 2x4
    // Orden: (0,0)=1, (0,1)=2, (0,2)=4, (1,0)=8, (1,1)=16, (1,2)=32, (0,3)=64, (1,3)=128
    static constexpr uint8_t BIT_WEIGHTS[4][2] = {
        {1,   8},   // Fila 0: Puntos 1, 4
        {2,  16},   // Fila 1: Puntos 2, 5
        {4,  32},   // Fila 2: Puntos 3, 6
        {64, 128}   // Fila 3: Puntos 7, 8
    };

    struct Config {
        Color foreground;
        Color background;
        bool smooth_edges = false; // Futuro: anti-aliasing simulado
        
        Config() : foreground(Color::RGB(255, 255, 255)), background(Color::RGB(0, 0, 0)) {}
    };

    /**
     * @brief Convierte una matriz de bits 2x4 en un carácter Braille.
     * @param pixels Matriz booleana de 4 filas x 2 columnas.
     * @return Carácter Unicode Braille.
     */
    static char32_t bits_to_braille(const bool pixels[4][2]);

    /**
     * @brief Renderiza una región de una imagen binaria a una cadena de caracteres Braille.
     * @param width Ancho de la imagen source en píxeles.
     * @param height Alto de la imagen source en píxeles.
     * @param pixels Puntero a datos de imagen (row-major, 1 byte por píxel: 0 o 1).
     * @param config Configuración de estilo.
     * @return String con los caracteres Braille renderizados.
     */
    static std::string render_image(int width, int height, const uint8_t* pixels, const Config& config = Config());

    /**
     * @brief Dibuja un punto en un buffer de caracteres Braille.
     * @param buffer Buffer de salida (ancho * alto caracteres).
     * @param buffer_width Ancho del buffer en caracteres Braille.
     * @param x Coordenada X del píxel (0-based).
     * @param y Coordenada Y del píxel (0-based).
     * @param value Estado del píxel (true = encendido).
     */
    static void plot_point(std::u32string& buffer, int buffer_width, int x, int y, bool value = true);

    /**
     * @brief Dibuja una línea entre dos puntos usando algoritmo de Bresenham adaptado a Braille.
     */
    static void draw_line(std::u32string& buffer, int buffer_width, int x0, int y0, int x1, int y1, bool value = true);

    /**
     * @brief Calcula las dimensiones necesarias en caracteres Braille para una imagen en píxeles.
     */
    static inline int required_width_px(int braille_cols) { return braille_cols * BRAILLE_WIDTH; }
    static inline int required_height_px(int braille_rows) { return braille_rows * BRAILLE_HEIGHT; }
    static inline int cols_for_pixels(int width_px) { return (width_px + BRAILLE_WIDTH - 1) / BRAILLE_WIDTH; }
    static inline int rows_for_pixels(int height_px) { return (height_px + BRAILLE_HEIGHT - 1) / BRAILLE_HEIGHT; }

private:
    // Helper para validar límites
    static bool is_valid_coord(int x, int y, int w, int h);
};

} // namespace tui
