# Contributing to Desktop TUI

Gracias por tu interés en contribuir a Desktop TUI! Este documento te guiará a través del proceso de contribución.

## ¿Cómo Contribuir?

### 1. Reportar Bugs

Si encuentras un bug, por favor crea un issue en GitHub con:
- Descripción clara del problema
- Pasos para reproducir
- Información del sistema (OS, terminal, versión)
- Salidas de errores

### 2. Sugerir Características

Para sugerencias de nuevas características:
- Explica el caso de uso
- Proporciona ejemplos de cómo funcionaría
- Considera el impacto en la arquitectura

### 3. Enviar Pull Requests

#### Flujo de Trabajo

1. **Fork** el repositorio
2. **Clona** tu fork: `git clone https://github.com/tu-usuario/desktop-tui.git`
3. **Crea una rama**: `git checkout -b feature/nueva-caracteristica`
4. **Haz cambios** y prueba localmente
5. **Commit** con mensajes claros: `git commit -m "Add: nueva característica"`
6. **Push**: `git push origin feature/nueva-caracteristica`
7. **Pull Request** en GitHub

#### Convenciones de Commit

Usa el formato [Conventional Commits](https://www.conventionalcommits.org/):

```
type(scope): description

[optional body]

[optional footer]
```

**Types:**
- `feat`: Nueva característica
- `fix`: Corrección de bug
- `docs`: Documentación
- `style`: Formato de código
- `refactor`: Refactorización
- `perf`: Mejoras de rendimiento
- `test`: Pruebas
- `chore`: Tareas de mantenimiento

**Ejemplos:**
```
feat(window): agregar soporte para maximizar ventanas
fix(colors): corregir cálculo de colores true color
docs(README): actualizar instrucciones de compilación
```

### 4. Estilos de Código

#### C++ Style

- Usa **C++17** features (std::optional, std::variant, structured bindings)
- Mantén archivos bajo 500 líneas
- Usa **RAII** para gestión de recursos
- Comenta el **porqué**, no el **qué**
- Mantén funciones pequeñas y con una sola responsabilidad

#### Ejemplo de Código

```cpp
// ✅ Bueno
auto process_event(Event& e) {
    if (e.type == EventType::KeyPress) {
        handle_keypress(e);
    }
}

// ❌ Malo
void handle_input() {
    // 200 líneas de código sin estructura
}
```

### 5. Pruebas

#### Escribir Pruebas

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Window drag works correctly", "[window][drag]") {
    auto win = std::make_shared<Window>("Test", Rect{0, 0, 10, 10});
    win->start_drag();
    
    // Simular movimiento del mouse
    Event e{EventType::MouseMove, 5, 5};
    win->handle_event(e);
    
    REQUIRE(win->bounds_x() == 5);
}
```

#### Ejecutar Pruebas

```bash
# Compilar con pruebas
mkdir build && cd build
cmake .. -DENABLE_TESTS=ON
make

# Ejecutar todas las pruebas
ctest

# Ejecutar prueba específica
ctest -R "window"
```

### 6. Revisión de Código

Los PRs serán revisados por:
- Correctitud del código
- Seguimiento de convenciones del proyecto
- Consideraciones de rendimiento
- Cobertura de pruebas
- Seguridad

### 7. Criterios de Aceptación

Los PRs deben:
- ✅ Pasar todas las pruebas
- ✅ No aumentar la complejidad ciclomática
- ✅ Incluir pruebas para nuevas características
- ✅ Tener mensajes de commit claros
- ✅ No incluir cambios no relacionados

### 8. Código de Conducta

Nuestro código de conducta es:
- **Respetuoso**: Trata a todos con respeto
- **Inclusivo**: Bienvenido a todos, independientemente
- **Constructivo**: Críticas al código, no a las personas

## Áreas de Contribución

### Alto Prioridad

1. **Pruebas**: Añadir más casos de prueba
2. **Documentación**: Mejorar la documentación existente
3. **Bugs conocidos**: Corregir issues reportados

### Medio Prioridad

1. **Características**: Mejoras de UX
2. **Rendimiento**: Optimizaciones
3. **Plataformas**: Soporte para más sistemas

### Bajo Prioridad

1. **Refactorización**: Mejorar código legacy
2. **Ejemplos**: Añadir más ejemplos
3. **Herramientas**: Mejorar scripts de desarrollo

## Preguntas Frecuentes

### ¿Necesito experiencia en C++?

Sí, pero no necesitas ser un experto. Familiarízate con C++17 antes de contribuir.

### ¿Puedo contribuir en español?

Los comentarios del código deben estar en inglés. La documentación puede estar en español o inglés.

### ¿Cómo reportar un bug?

Crea un issue en GitHub con:
- Descripción clara
- Pasos para reproducir
- Información del sistema

### ¿Cuál es el mejor lugar para empezar?

1. Busca issues etiquetados como `good first issue`
2. Revisa la documentación existente
3. Compila y prueba localmente

## Recursos

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [GitHub Pull Request Guide](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests)
- [CMake Documentation](https://cmake.org/documentation/)

## Contacto

- Issues: GitHub Issues
- Discusión: GitHub Discussions
- Email: maintainer@example.com

¡Gracias por contribuir! 🎉
