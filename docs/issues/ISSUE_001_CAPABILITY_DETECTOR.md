# Issue Técnico: Capability Detector

## Descripción
Implementar sistema de detección runtime de capacidades del terminal para habilitar características gráficas avanzadas de forma dinámica y segura.

## Objetivos
1. Detectar automáticamente soporte para Braille, Sixel, Kitty Graphics
2. Consultar Device Attributes (DA1/DA2) vía secuencias ESC
3. Proveer fallback graceful a capacidades básicas
4. Cache de resultados para evitar re-detección

## Archivos a Crear
- `include/core/capability_detector.hpp`
- `src/core/capability_detector.cpp`
- `tests/test_capability_detector.cpp`

## API Propuesta
```cpp
namespace tui {

enum class GraphicsCaps : uint32_t {
    None          = 0,
    Braille       = 1 << 0,
    ASCIIGraphics = 1 << 1,
    Sixel         = 1 << 2,
    Kitty         = 1 << 3,
    iTerm2        = 1 << 4,
};

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
    static bool query_da1();
    static bool query_da2();
    static bool test_braille();
    static bool test_sixel();
};

} // namespace tui
```

## Criterios de Aceptación
- [ ] Detección completa en <100ms
- [ ] Zero memory leaks (verificado con ASan)
- [ ] 100% code coverage en tests
- [ ] Funciona en Linux, macOS, Windows, Termux
- [ ] Documentación completa de API

## Prioridad: Alta
## Estimación: 2-3 días
## Asignado: Pendiente
