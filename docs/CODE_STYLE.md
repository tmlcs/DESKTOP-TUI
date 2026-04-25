# Code Style Guide

Esta guía establece las convenciones de código para mantener la consistencia y legibilidad en Desktop TUI.

## C++ Style

### 1. Nombres y Comentario

#### Nombres de Variables y Funciones

- Usa **snake_case** para variables y funciones
- Usa **PascalCase** para clases y tipos
- Usa **UPPER_SNAKE_CASE** para constantes

```cpp
// ✅ Bueno
auto window_width = calculate_window_width();
const int MAX_WINDOWS = 20;

// ❌ Malo
auto windowWidth = calculateWindowWidth();
const int maxWindows = 20;
```

#### Comentario

- Comenta el **porqué**, no el **qué**
- Usa bloques de comentario para secciones grandes
- Comenta funciones complejas con su propósito

```cpp
// ✅ Bueno
// Calculate the optimal window position based on screen bounds
// and avoid overlapping with other windows
auto position = calculate_optimal_position(bounds, other_windows);

// ❌ Malo
// Calculate position
auto position = calculate_optimal_position(bounds, other_windows);
```

### 2. Estructura de Funciones

#### Funciones Pequeñas

- Mantén funciones bajo 50 líneas
- Una función = una responsabilidad
- Extrae funciones para lógica compleja

```cpp
// ✅ Bueno
void handle_keypress(Event& e) {
    if (e.type == EventType::KeyPress) {
        handle_desktop_switch(e);
    }
}

void handle_desktop_switch(Event& e) {
    // Lógica específica
}

// ❌ Malo
void handle_keypress(Event& e) {
    // 100 líneas de código sin estructura
}
```

#### Parámetros

- Usa nombres descriptivos
- Máximo 3-4 parámetros por función
- Usa `std::optional` para parámetros opcionales

```cpp
// ✅ Bueno
void set_window_title(Window& win, std::string title);
void create_window(std::string title, Rect bounds, bool focusable = false);

// ❌ Malo
void set_title(w, t);
void create(w, t, b, f);
```

### 3. Control de Flujo

#### If/Else

- Usa `{}` incluso para un solo statement
- Alinea `else` con `if`
- Usa `return early` para reducir anidación

```cpp
// ✅ Bueno
void process_event(Event& e) {
    if (e.type == EventType::KeyPress) {
        handle_keypress(e);
        return;
    }
    
    if (e.type == EventType::MouseMove) {
        handle_mouse_move(e);
        return;
    }
    
    // Default case
}

// ❌ Malo
void process_event(Event& e) {
    if (e.type == EventType::KeyPress) {
        handle_keypress(e);
    } else {
        // ...
    }
}
```

#### Switch

- Usa `switch` para múltiples valores
- Usa `[[fallthrough]]` cuando sea apropiado
- Usa `default` siempre

```cpp
void handle_event_type(EventType type) {
    switch (type) {
        case EventType::KeyPress:
            handle_keypress();
            break;
        case EventType::MouseMove:
            handle_mouse_move();
            break;
        case EventType::Resize:
            handle_resize();
            [[fallthrough]];
        default:
            handle_unknown();
            break;
    }
}
```

### 4. Smart Pointers

#### Uso de Smart Pointers

- Usa `std::shared_ptr` cuando hay múltiples owners
- Usa `std::unique_ptr` para ownership exclusivo
- Evita cycles de referencia

```cpp
// ✅ Bueno
auto window = std::make_shared<Window>(title, bounds);
auto panel = std::make_shared<Panel>(style);
window->set_content(panel);

// ❌ Malo
Window* window = new Window(title, bounds);
// Olvidaste el delete
```

### 5. Manejo de Errores

#### Validación de Entrada

- Valida entrada en los límites del sistema
- Usa `std::optional` para resultados fallidos
- Lanza excepciones para errores de programación

```cpp
// ✅ Bueno
std::optional<Window> create_window(std::string title, Rect bounds) {
    if (bounds.w < Config::MIN_WINDOW_WIDTH) {
        return std::nullopt;
    }
    return std::make_optional<Window>(title, bounds);
}

// ❌ Malo
Window* create_window(std::string title, Rect bounds) {
    // No validación
    return new Window(title, bounds);
}
```

### 6. Formateo de Código

#### Estilo de Línea

- Máximo 80 caracteres por línea
- Usa 2 espacios para indentación
- Una declaración por línea

```cpp
// ✅ Bueno
std::vector<std::shared_ptr<Window>> windows;
windows.reserve(100);

for (auto& win : windows) {
    win->render(renderer);
}

// ❌ Malo
std::vector<std::shared_ptr<Window>> windows;
windows.reserve(100);
for (auto& win : windows) { win->render(renderer); }
```

#### Espaciado

- Espacio alrededor de operadores binarios
- Sin espacios en operadores unarios
- Espacio antes de `{}` en llamadas de función

```cpp
// ✅ Bueno
auto result = calculate_value(x + y, z * w);
if (condition) {
    // code
}

// ❌ Malo
auto result=calculate_value(x+y,z*w);
if(condition) { code }
```

### 7. Headers

#### Organización de Includes

1. Includes del sistema
2. Includes de terceros
3. Includes locales (ordenados alfabéticamente)

```cpp
// ✅ Bueno
#include <memory>
#include <string>
#include <vector>

#include "core/config.hpp"
#include "core/rect.hpp"
#include "ui/panel.hpp"

// ❌ Malo
#include "ui/panel.hpp"
#include <memory>
#include <string>
#include "core/config.hpp"
```

### 8. Pruebas

#### Estructura de Pruebas

- Usa Catch2
- Una prueba por caso
- Pruebas pequeñas y enfocadas

```cpp
TEST_CASE("Window drag works correctly", "[window][drag]") {
    auto win = std::make_shared<Window>("Test", Rect{0, 0, 10, 10});
    win->start_drag();
    
    Event e{EventType::MouseMove, 5, 5};
    win->handle_event(e);
    
    REQUIRE(win->bounds_x() == 5);
}
```

### 9. Documentación

#### Doxygen

- Documenta funciones públicas
- Documenta parámetros y retorno
- Usa ejemplos cuando sea necesario

```cpp
/**
 * @brief Crea una nueva ventana
 * 
 * @param title Título de la ventana
 * @param bounds Bordes de la ventana
 * @param focusable Si la ventana puede recibir foco
 * @return std::optional<Window> La ventana creada o std::nullopt si falla
 * 
 * @note La ventana debe tener al menos MIN_WINDOW_WIDTH x MIN_WINDOW_HEIGHT
 */
std::optional<Window> create_window(
    std::string title,
    Rect bounds,
    bool focusable = false
) {
    // ...
}
```

### 10. Revisión de Código

#### Checklist

Antes de hacer commit:

- [ ] Código formateado con `clang-format`
- [ ] Pruebas pasan
- [ ] Comentarios actualizados
- [ ] Sin warnings de compilación
- [ ] Código revisado por otra persona

#### Herramientas

- **clang-format**: Formateo de código
- **clang-tidy**: Análisis de código
- **cppcheck**: Análisis estático
- **valgrind**: Detección de fugas de memoria

```bash
# Formatear todo el proyecto
find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i

# Analizar con clang-tidy
clang-tidy -p build ./src

# Analizar con cppcheck
cppcheck --enable=all --inline-suppr .
```

## Conclusión

Sigue estas guías para mantener un código consistente y legible. La consistencia es más importante que el estilo específico.
