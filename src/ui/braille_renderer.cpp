#include "ui/braille_renderer.h"
#include <cmath>
#include <algorithm>

namespace tui {

char32_t BrailleRenderer::bits_to_braille(const bool pixels[4][2]) {
    uint8_t value = 0;
    
    for (int row = 0; row < BRAILLE_HEIGHT; ++row) {
        for (int col = 0; col < BRAILLE_WIDTH; ++col) {
            if (pixels[row][col]) {
                value += BIT_WEIGHTS[row][col];
            }
        }
    }
    
    return static_cast<char32_t>(BRAILLE_BASE + value);
}

std::string BrailleRenderer::render_image(int width, int height, const uint8_t* pixels, const Config& config) {
    if (!pixels || width <= 0 || height <= 0) {
        return "";
    }

    int cols = cols_for_pixels(width);
    int rows = rows_for_pixels(height);
    
    // Inicializar con carácter Braille vacío (U+2800), no espacio normal
    std::u32string buffer(cols * rows, U'\u2800');
    
    // Procesar cada píxel de la imagen source
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            uint8_t val = pixels[y * width + x];
            if (val > 0) {
                plot_point(buffer, cols, x, y, true);
            }
        }
    }
    
    // Convertir u32string a UTF-8 string
    std::string result;
    result.reserve(cols * rows * 4); // Máximo 4 bytes por carácter UTF-8
    
    for (char32_t ch : buffer) {
        if (ch < 0x80) {
            result.push_back(static_cast<char>(ch));
        } else if (ch < 0x800) {
            result.push_back(static_cast<char>(0xC0 | (ch >> 6)));
            result.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        } else if (ch < 0x10000) {
            result.push_back(static_cast<char>(0xE0 | (ch >> 12)));
            result.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        } else {
            result.push_back(static_cast<char>(0xF0 | (ch >> 18)));
            result.push_back(static_cast<char>(0x80 | ((ch >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        }
    }
    
    return result;
}

void BrailleRenderer::plot_point(std::u32string& buffer, int buffer_width, int x, int y, bool value) {
    if (x < 0 || y < 0) return;
    
    int col = x / BRAILLE_WIDTH;
    int row = y / BRAILLE_HEIGHT;
    
    if (col >= buffer_width || row * buffer_width + col >= static_cast<int>(buffer.size())) {
        return;
    }
    
    int local_x = x % BRAILLE_WIDTH;
    int local_y = y % BRAILLE_HEIGHT;
    
    char32_t current = buffer[row * buffer_width + col];
    
    // Si el carácter actual no es un carácter Braille válido, inicializarlo
    uint8_t current_bits;
    if (current < BRAILLE_BASE || current > BRAILLE_BASE + 0xFF) {
        current_bits = 0;
    } else {
        current_bits = static_cast<uint8_t>(current - BRAILLE_BASE);
    }
    
    if (value) {
        current_bits |= BIT_WEIGHTS[local_y][local_x];
    } else {
        current_bits &= ~BIT_WEIGHTS[local_y][local_x];
    }
    
    buffer[row * buffer_width + col] = static_cast<char32_t>(BRAILLE_BASE + current_bits);
}

bool BrailleRenderer::is_valid_coord(int x, int y, int w, int h) {
    return x >= 0 && x < w && y >= 0 && y < h;
}

void BrailleRenderer::draw_line(std::u32string& buffer, int buffer_width, int x0, int y0, int x1, int y1, bool value) {
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    
    while (true) {
        plot_point(buffer, buffer_width, x0, y0, value);
        
        if (x0 == x1 && y0 == y1) break;
        
        int e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

} // namespace tui
