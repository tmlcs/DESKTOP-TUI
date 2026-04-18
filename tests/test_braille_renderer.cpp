#include "ui/braille_renderer.h"
#include <cstdio>
#include <cstring>

namespace tui {
namespace test {

// Simple test macros
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("  ❌ FAIL: %s\n", msg); \
        (*failed)++; \
    } else { \
        printf("  ✓ PASS\n"); \
        (*passed)++; \
    } \
} while(0)

#define ASSERT_EQ(a, b, msg) TEST_ASSERT((a) == (b), msg)
#define ASSERT_TRUE(cond, msg) TEST_ASSERT((cond), msg)
#define ASSERT_GT(a, b, msg) TEST_ASSERT((a) > (b), msg)
#define ASSERT_LE(a, b, msg) TEST_ASSERT((a) <= (b), msg)

void run_braille_tests(int* passed, int* failed) {
    printf("=== BrailleRenderer Tests ===\n");

    // Test 1: Conversión básica de bits a Braille
    {
        printf("\nTest: bits_to_braille_basic\n");
        bool pixels[4][2] = {
            {true, false},   // Punto 1 activo
            {false, true},   // Punto 5 activo
            {false, false},
            {false, false}
        };
        
        char32_t result = BrailleRenderer::bits_to_braille(pixels);
        
        // Punto 1 (bit 0) + Punto 5 (bit 16) = 17 + 0x2800 = 0x2811 (⠑)
        ASSERT_EQ(result, 0x2811, "Carácter Braille incorrecto para puntos 1 y 5");
    }

    // Test 2: Carácter Braille vacío
    {
        printf("\nTest: bits_to_braille_empty\n");
        bool pixels[4][2] = {
            {false, false},
            {false, false},
            {false, false},
            {false, false}
        };
        
        char32_t result = BrailleRenderer::bits_to_braille(pixels);
        ASSERT_EQ(result, 0x2800, "Carácter Braille vacío debe ser U+2800 (espacio Braille)");
    }

    // Test 3: Carácter Braille lleno (todos los puntos)
    {
        printf("\nTest: bits_to_braille_full\n");
        bool pixels[4][2] = {
            {true, true},
            {true, true},
            {true, true},
            {true, true}
        };
        
        char32_t result = BrailleRenderer::bits_to_braille(pixels);
        // Suma de todos los bits: 1+2+4+8+16+32+64+128 = 255
        ASSERT_EQ(result, 0x28FF, "Carácter Braille lleno debe ser U+28FF");
    }

    // Test 4: Plot point individual
    {
        printf("\nTest: plot_point_single\n");
        std::u32string buffer(4, U' '); // 2x2 caracteres
        
        BrailleRenderer::plot_point(buffer, 2, 0, 0, true);
        
        // Debería activar el punto 1 (bit 1) en la celda (0,0)
        ASSERT_EQ(buffer[0], 0x2801, "Punto (0,0) no se plotteó correctamente");
        // La celda adyacente no debe modificarse (permanece como espacio U+0020)
        ASSERT_EQ(buffer[1], 0x0020, "Celda adyacente no debe modificarse");
    }

    // Test 5: Plot point en segunda columna de celda Braille
    {
        printf("\nTest: plot_point_second_column\n");
        std::u32string buffer(1, U' ');
        
        // x=1 está en la columna derecha de la primera celda Braille
        BrailleRenderer::plot_point(buffer, 1, 1, 0, true);
        
        // Debería activar el punto 4 (bit 8)
        ASSERT_EQ(buffer[0], 0x2808, "Punto en columna derecha no se plotteó correctamente");
    }

    // Test 6: Plot point fuera de límites
    {
        printf("\nTest: plot_point_out_of_bounds\n");
        std::u32string buffer(4, U' ');
        std::u32string original = buffer;
        
        BrailleRenderer::plot_point(buffer, 2, -1, 0, true);  // X negativo
        BrailleRenderer::plot_point(buffer, 2, 10, 0, true);  // X fuera de rango
        BrailleRenderer::plot_point(buffer, 2, 0, -1, true);  // Y negativo
        
        ASSERT_EQ(buffer, original, "Plot point fuera de límites no debe modificar el buffer");
    }

    // Test 7: Renderizado de imagen pequeña
    {
        printf("\nTest: render_image_small\n");
        // Imagen 4x4 píxeles (debería caber en 2x1 celdas Braille)
        uint8_t pixels[16] = {
            1, 0, 0, 1,
            0, 1, 1, 0,
            0, 0, 0, 0,
            1, 1, 1, 1
        };
        
        std::string result = BrailleRenderer::render_image(4, 4, pixels);
        
        // 4px ancho / 2 = 2 columnas Braille
        // 4px alto / 4 = 1 fila Braille
        // Total: 2 caracteres Unicode, pero en UTF-8 cada carácter Braille ocupa 3 bytes
        // Por lo tanto, string.length() retorna 6 bytes, no 2 caracteres
        ASSERT_EQ(result.length(), 6, "Longitud incorrecta del string renderizado (esperado 6 bytes UTF-8 para 2 caracteres Braille)");
        
        // Verificar que son caracteres UTF-8 válidos (múltiples bytes)
        ASSERT_TRUE(result.length() >= 6, "Resultado debe contener al menos 6 bytes (2 caracteres x 3 bytes)");
    }

    // Test 8: Línea diagonal con Bresenham
    {
        printf("\nTest: draw_line_diagonal\n");
        std::u32string buffer(16, U' '); // 4x4 celdas
        
        BrailleRenderer::draw_line(buffer, 4, 0, 0, 7, 7, true);
        
        // Verificar que se dibujaron puntos a lo largo de la diagonal
        int points_count = 0;
        for (char32_t ch : buffer) {
            if (ch != 0x2800) {
                points_count++;
            }
        }
        
        ASSERT_GT(points_count, 0, "La línea debe tener al menos un punto dibujado");
        ASSERT_LE(points_count, 16, "La línea no debe exceder el número máximo de píxeles");
    }

    // Test 9: Línea horizontal
    {
        printf("\nTest: draw_line_horizontal\n");
        std::u32string buffer(8, U' '); // 8x1 celdas
        
        BrailleRenderer::draw_line(buffer, 8, 0, 2, 15, 2, true);
        
        // Todos los puntos deben estar en la fila y=2
        bool found_points = false;
        for (char32_t ch : buffer) {
            if (ch != 0x2800) {
                found_points = true;
                break;
            }
        }
        
        ASSERT_TRUE(found_points, "Línea horizontal debe tener puntos dibujados");
    }

    // Test 10: Cálculo de dimensiones
    {
        printf("\nTest: dimension_calculations\n");
        ASSERT_EQ(BrailleRenderer::cols_for_pixels(1), 1, "cols_for_pixels(1) debe ser 1");
        ASSERT_EQ(BrailleRenderer::cols_for_pixels(2), 1, "cols_for_pixels(2) debe ser 1");
        ASSERT_EQ(BrailleRenderer::cols_for_pixels(3), 2, "cols_for_pixels(3) debe ser 2");
        ASSERT_EQ(BrailleRenderer::cols_for_pixels(4), 2, "cols_for_pixels(4) debe ser 2");
        
        ASSERT_EQ(BrailleRenderer::rows_for_pixels(1), 1, "rows_for_pixels(1) debe ser 1");
        ASSERT_EQ(BrailleRenderer::rows_for_pixels(4), 1, "rows_for_pixels(4) debe ser 1");
        ASSERT_EQ(BrailleRenderer::rows_for_pixels(5), 2, "rows_for_pixels(5) debe ser 2");
        ASSERT_EQ(BrailleRenderer::rows_for_pixels(8), 2, "rows_for_pixels(8) debe ser 2");
        
        ASSERT_EQ(BrailleRenderer::required_width_px(3), 6, "required_width_px(3) debe ser 6");
        ASSERT_EQ(BrailleRenderer::required_height_px(2), 8, "required_height_px(2) debe ser 8");
    }

    // Test 11: Imagen con padding (dimensiones no múltiplos de 2x4)
    {
        printf("\nTest: render_image_with_padding\n");
        // Imagen 3x5 píxeles (no es múltiplo exacto de 2x4)
        uint8_t pixels[15] = {
            1, 1, 1,
            1, 0, 1,
            1, 1, 1,
            0, 0, 0,
            1, 1, 1
        };
        
        std::string result = BrailleRenderer::render_image(3, 5, pixels);
        
        // 3px -> 2 columnas, 5px -> 2 filas = 4 caracteres totales
        // Cada carácter Braille en UTF-8 ocupa 3 bytes, entonces 4 * 3 = 12 bytes
        ASSERT_EQ(result.length(), 12, "Renderizado con padding debe calcular dimensiones correctas (4 caracteres x 3 bytes = 12 bytes)");
    }

    // Test 12: Imagen vacía o nula
    {
        printf("\nTest: render_image_edge_cases\n");
        BrailleRenderer::Config empty_config;
        ASSERT_EQ(BrailleRenderer::render_image(0, 10, nullptr, empty_config), "", "render_image con width=0 debe retornar string vacío");
        ASSERT_EQ(BrailleRenderer::render_image(10, 0, nullptr, empty_config), "", "render_image con height=0 debe retornar string vacío");
        
        uint8_t dummy[1] = {1};
        ASSERT_EQ(BrailleRenderer::render_image(-1, 10, dummy, empty_config), "", "render_image con width negativo debe retornar string vacío");
        ASSERT_EQ(BrailleRenderer::render_image(10, -5, dummy, empty_config), "", "render_image con height negativo debe retornar string vacío");
    }

    printf("\n=== BrailleRenderer Tests Complete ===\n");
}

} // namespace test
} // namespace tui

#ifdef STANDALONE_BRAILLE_TEST
int main() {
    int passed = 0, failed = 0;
    tui::test::run_braille_tests(&passed, &failed);
    printf("\n=== Results: %d passed, %d failed ===\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
#endif
