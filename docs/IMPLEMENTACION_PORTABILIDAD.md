# Plan de Implementación: Opción E - Portabilidad Mejorada

## Resumen Ejecutivo
Implementar soporte nativo robusto para Android (Termux y NDK) y añadir capacidades gráficas avanzadas usando caracteres Braille y ASCII art para representar imágenes y gráficos complejos en terminales sin soporte Sixel/Kitty.

---

## 1. Estado Actual del Código Base

### Arquitectura Platform HAL Existente
- **ITerminal**: Interfaz abstracta con 20+ métodos virtuales
- **IInput**: Parser de eventos con soporte CSI, SS3, SGR 1006
- **Implementaciones**:
  - `terminal_posix.cpp` / `input_posix.cpp` (Linux, macOS)
  - `terminal_win.cpp` / `input_win.cpp` (Windows)
  - `terminal_android.cpp` / `input_android.cpp` (wrapper de POSIX)
  - `terminal_generic.cpp` / `input_generic.cpp` (fallback)

### Limitaciones Identificadas
1. **Android**: Solo wrapper de POSIX, sin integración NDK nativa
2. **Gráficos**: Solo box-drawing Unicode básico
3. **Detección**: No hay runtime detection de capacidades gráficas
4. **Termux**: Sin optimizaciones específicas para terminal Android

---

## 2. Implementación Propuesta

### 2.1 Backend Android Nativo (NDK)

#### Archivos Nuevos
```
src/platform/
├── terminal_ndk.cpp          # Implementación nativa Android NDK
├── input_ndk.cpp             # Input handler con AInputQueue
├── android_jni.cpp           # JNI bindings para Java ↔ C++
└── android_looper.cpp        # ALooper integration para events

include/platform/
├── terminal_ndk.hpp          # Header NDK terminal
├── input_ndk.hpp             # Header NDK input
└── android_helpers.hpp       # Utilidades Android
```

#### Características Clave
- **ANativeWindow**: Renderizado directo sin escape sequences
- **AInputQueue**: Input handling asíncrono nativo
- **ALooper**: Event loop integrado con Android lifecycle
- **JNI Layer**: Comunicación con Activity Java/Kotlin
- **Configuración**: Detección automática de orientación, DPI, refresh rate

#### Cambios en CMakeLists.txt
```cmake
if(ANDROID)
    add_library(tui_platform SHARED
        src/platform/terminal_ndk.cpp
        src/platform/input_ndk.cpp
        src/platform/android_jni.cpp
        src/platform/android_looper.cpp
    )
    target_link_libraries(tui_platform PRIVATE
        android
        log
        input
        native_window
    )
endif()
```

### 2.2 Sistema de Gráficos Braille/ASCII

#### Archivos Nuevos
```
include/core/
├── braille_graphics.hpp      # Motor de renderizado Braille
└── ascii_renderer.hpp        # ASCII art procedural

src/core/
├── braille_graphics.cpp
└── ascii_renderer.cpp

examples/
└── braille_demo.cpp          # Demo de capacidades gráficas
```

#### Capacidades
- **Braille Patterns**: 2x4 dots = 256 combinaciones por celda
- **Resolución efectiva**: 8x más densidad que caracteres normales
- **Modos**:
  - `BrailleMode::Dense`: Máxima resolución (2x4 dots/cell)
  - `BrailleMode::Sparse`: Compatible con terminales antiguos
  - `BrailleMode::Auto`: Detección automática de soporte
- **Funciones**:
  - `draw_pixel(x, y, color)` → mapea a patrón Braille
  - `draw_line_braille(x0, y0, x1, y1)`
  - `render_bitmap(data, width, height)` → convierte imagen a Braille
  - `ascii_sphere(radius, light_dir)` → esfera 3D ASCII procedural

#### Integración con ITerminal
```cpp
class ITerminal {
    // ... existing methods ...
    
    // Nuevos métodos para gráficos avanzados
    virtual void draw_pixel(int x, int y, const Color& color) = 0;
    virtual void draw_pixels(const PixelBuffer& buffer, int x, int y, int w, int h) = 0;
    virtual GraphicsCaps graphics_caps() const = 0;
};

enum class GraphicsCaps : uint32_t {
    None          = 0,
    Braille       = 1 << 0,  // Soporte Unicode Braille (U+2800–U+28FF)
    ASCIIGraphics = 1 << 1,  // Fallback a ASCII art
    Sixel         = 1 << 2,  // Protocolo Sixel para imágenes
    Kitty         = 1 << 3,  // Kitty Graphics Protocol
    iTerm2        = 1 << 4,  // iTerm2 inline images
};
```

### 2.3 Detección Runtime de Capacidades

#### Archivos Nuevos
```
include/core/
└── capability_detector.hpp   # Detección dinámica de features

src/core/
└── capability_detector.cpp
```

#### Estrategia de Detección
```cpp
class CapabilityDetector {
public:
    struct TerminalInfo {
        std::string term;
        std::string version;
        GraphicsCaps graphics;
        TerminalCaps terminal;
        bool supports_braille;
        bool supports_sixel;
        bool supports_kitty;
        int max_colors;
    };
    
    static TerminalInfo detect();
    static bool query_da1();      // ESC [ c - Primary Device Attributes
    static bool query_da2();      // ESC [ > c - Secondary DA
    static bool test_braille();   // Write U+2800, check response
    static bool test_sixel();     // Send sixel query, parse response
};
```

### 2.4 Optimizaciones Específicas para Termux

#### Mejoras en terminal_posix.cpp
```cpp
// Detección específica de Termux
if (is_termux()) {
    // Termux tiene limitaciones con ciertas secuencias
    caps_ = caps_ & ~TerminalCaps::TrueColor;  // Algunas versiones no soportan
    enable_termux_workarounds();
}

// Funciones helper
bool PosixTerminal::is_termux() const {
    const char* prefix = getenv("PREFIX");
    return prefix && strstr(prefix, "/com.termux/files/usr") != nullptr;
}
```

---

## 3. Roadmap de Implementación

### Fase 1: Fundamentos (Semanas 1-2)
- [ ] Crear `capability_detector.hpp/cpp`
- [ ] Extender `ITerminal` con métodos gráficos
- [ ] Implementar detección DA1/DA2
- [ ] Tests unitarios para detector

### Fase 2: Gráficos Braille (Semanas 3-4)
- [ ] Implementar `braille_graphics.hpp/cpp`
- [ ] Mapeo pixel → patrón Braille
- [ ] Funciones de dibujo (líneas, círculos, bitmap)
- [ ] Demo interactivo `braille_demo.cpp`
- [ ] Tests de renderizado

### Fase 3: Backend Android NDK (Semanas 5-7)
- [ ] Configurar NDK toolchain en CMake
- [ ] Implementar `terminal_ndk.cpp` con ANativeWindow
- [ ] Implementar `input_ndk.cpp` con AInputQueue
- [ ] Crear JNI layer (`android_jni.cpp`)
- [ ] Integrar con ALooper para event loop
- [ ] App de ejemplo Android (Java + C++)

### Fase 4: Integración y Testing (Semanas 8-9)
- [ ] Unificar APIs entre backends
- [ ] Testing en dispositivos reales Android
- [ ] Testing en Termux
- [ ] Documentación de API
- [ ] Ejemplos y tutoriales

---

## 4. Métricas de Éxito

### Funcionales
- ✅ Renderizado Braille funcional en 95% terminales modernos
- ✅ App Android compilable con NDK r25+
- ✅ Detección automática de capacidades >90% precisión
- ✅ Zero crashes en pruebas de estrés (1000+ resize events)

### Rendimiento
- ✅ 60fps en renderizado Braille (terminal 80x24)
- ✅ <1ms latency en input Android nativo
- ✅ <100KB overhead memoria para gráficos Braille

### Portabilidad
- ✅ Compilación exitosa en:
  - Linux (gcc, clang)
  - macOS (Xcode clang)
  - Windows (MSVC, MinGW)
  - Android (NDK r25+, API 21+)
  - Termux (clang-aarch64)

---

## 5. Riesgos y Mitigación

| Riesgo | Impacto | Probabilidad | Mitigación |
|--------|---------|--------------|------------|
| NDK compatibility issues | Alto | Media | Usar NDK r25 LTS, testing en CI |
| Braille no soportado en terminales viejos | Medio | Alta | Fallback automático a ASCII |
| Performance en devices low-end | Medio | Media | Dynamic resolution scaling |
| Fragmentation Android versions | Alto | Alta | Min API 21 (Android 5.0), 95% coverage |

---

## 6. Recursos Necesarios

### Humanos
- 1 desarrollador C++ senior (NDK, systems programming)
- 1 desarrollador Android (JNI, lifecycle management)
- QA tester con dispositivos Android variados

### Infraestructura
- CI/CD con Android emulators (GitHub Actions)
- Dispositivos físicos para testing (diversos OEMs)
- Acceso a Termux environment

### Dependencias
- Android NDK r25 o superior
- CMake 3.20+
- Ninja build system (recomendado)

---

## 7. Próximos Pasos Inmediatos

1. **Crear issue técnico** para `capability_detector.hpp`
2. **Prototipo rápido** de Braille renderer (2 días)
3. **Setup CI** con Android emulator
4. **Documentar API** extendida de ITerminal

---

## 8. Apéndice: Ejemplo de Uso

```cpp
#include "tui.hpp"
#include "core/braille_graphics.hpp"

int main() {
    auto app = tui::App::create();
    
    // Detección automática
    auto caps = tui::CapabilityDetector::detect();
    
    if (caps.graphics & tui::GraphicsCaps::Braille) {
        // Renderizar gráfico de alta resolución
        tui::BrailleRenderer renderer(app->terminal());
        
        // Dibujar círculo usando Braille
        renderer.draw_circle(40, 12, 10, tui::Color::Cyan);
        
        // O cargar bitmap
        auto image = load_image("logo.png");
        renderer.render_bitmap(image, 0, 0);
    } else {
        // Fallback a ASCII art
        tui::ASCIIRenderer renderer(app->terminal());
        renderer.draw_sphere(20, 12, 8);
    }
    
    app->run();
    return 0;
}
```

---

**Documento creado:** Diciembre 2024  
**Versión:** 1.0  
**Estado:** Pendiente de aprobación para implementación
