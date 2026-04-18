# Issue Técnico: Braille Graphics Renderer

## Descripción
Implementar motor de renderizado gráfico usando patrones Braille Unicode para lograr 8x más densidad de píxeles en terminales.

## Objetivos
1. Mapear píxeles a patrones Braille (2x4 dots = 256 combinaciones)
2. Implementar primitivas gráficas: líneas, círculos, rectángulos, bitmap
3. Soporte para colores por celda Braille
4. Fallback automático a ASCII si no hay soporte Unicode

## Archivos a Crear
- `include/core/braille_graphics.hpp`
- `src/core/braille_graphics.cpp`
- `include/core/ascii_renderer.hpp`
- `src/core/ascii_renderer.cpp`
- `examples/braille_demo.cpp`

## API Propuesta
```cpp
namespace tui {

enum class BrailleMode {
    Dense,   // 2x4 dots/cell, máxima resolución
    Sparse,  // Compatible con terminales antiguos
    Auto     // Detección automática
};

class BrailleRenderer {
public:
    explicit BrailleRenderer(ITerminal* terminal);
    
    // Configuración
    void set_mode(BrailleMode mode);
    BrailleMode mode() const;
    
    // Primitivas básicas
    void draw_pixel(int x, int y, const Color& color);
    void draw_line(int x0, int y0, int x1, int y1, const Color& color);
    void draw_circle(int cx, int cy, int radius, const Color& color);
    void draw_rect(int x, int y, int w, int h, const Color& color);
    
    // Bitmap rendering
    void render_bitmap(const uint8_t* data, int width, int height, 
                       int dst_x, int dst_y);
    void render_bitmap_rgba(const uint32_t* data, int width, int height,
                            int dst_x, int dst_y);
    
    // Utilidades
    void clear();
    void flush();
    
private:
    ITerminal* terminal_;
    BrailleMode mode_;
    // Buffer interno para acumular cambios antes de flush
    std::vector<uint8_t> cell_buffer_;
    int buffer_cols_, buffer_rows_;
};

// Fallback renderer para terminales sin Braille
class ASCIIRenderer {
public:
    explicit ASCIIRenderer(ITerminal* terminal);
    
    // Primitivas ASCII art
    void draw_sphere(int cx, int cy, int radius);
    void draw_cube(int x, int y, int size);
    void draw_gradient_h(int x, int y, int w, const Color& start, 
                         const Color& end);
    
private:
    ITerminal* terminal_;
};

} // namespace tui
```

## Algoritmo Clave: Mapeo Pixel → Braille
```
Celda Braille (1 carácter Unicode) = 2x4 dots:
⣿ U+28FF = todos los dots activos
⡇ U+2807 = solo columna derecha
etc.

Mapeo:
- Dividir coordenadas pantalla por 2 (ancho) y 4 (alto)
- Para cada celda, calcular patrón de 8 bits
- Convertir a código Unicode U+2800 + patrón
```

## Criterios de Aceptación
- [ ] Renderizado de imagen 80x24 celdas = 160x96 píxeles efectivos
- [ ] 60fps sostenidos en terminal moderno
- [ ] Zero allocations en render loop steady-state
- [ ] Fallback a ASCII funciona correctamente
- [ ] Tests visuales con screenshots comparativos

## Dependencias
- Ninguna externa (solo C++17 estándar)
- Requiere terminal con soporte Unicode (la mayoría modernos)

## Prioridad: Alta
## Estimación: 3-4 días
## Asignado: Pendiente
