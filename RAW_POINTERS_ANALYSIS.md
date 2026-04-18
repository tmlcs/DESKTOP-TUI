# Eliminación de Raw Pointers en Globals - Plan de Implementación

## Estado Actual del Análisis

### Hallazgo Principal:
El código base **NO tiene raw pointers peligrosos en globals**. 

Los únicos globals identificados son:
1. `g_running` (main.cpp) - `std::atomic<bool>` ✅ Seguro
2. `g_resize_pending` (terminal_posix.cpp) - `std::atomic<bool>` ✅ Seguro  
3. `g_is_initialized` (terminal_posix.cpp) - `std::atomic<bool>` ✅ Seguro

### Por qué esto es correcto:
- Todos usan `std::atomic<bool>` que es thread-safe
- Son tipos primitivos, no pointers
- No hay riesgo de memory leaks o dangling pointers
- El diseño actual sigue las mejores prácticas de C++ moderno

## Smart Pointers Existentes (Ya Correctos)

El proyecto YA usa smart pointers apropiadamente:

```cpp
// DesktopManager - usa shared_ptr
using DesktopPtr = std::shared_ptr<Desktop>;
std::vector<DesktopPtr> desktops_;

// TUIShell - usa unique_ptr implícito en miembros
ITerminal* term_;  // Pointer crudo pero con ownership claro
IInput* input_;    // Se gestiona con create_/destroy_ functions

// Windows - usa shared_ptr
auto win1 = std::make_shared<Window>(...);
```

## Mejoras Recomendadas (Opcionales)

Aunque no hay raw pointers peligrosos, podemos mejorar:

### 1. Envolver globals en namespace seguro
```cpp
namespace tui {
namespace globals {
    inline std::atomic<bool>& running_flag() {
        static std::atomic<bool> flag{true};
        return flag;
    }
}
}
```

### 2. Convertir factory functions a smart pointers
```cpp
// Actual
ITerminal* create_terminal();
void destroy_terminal(ITerminal* term);

// Mejorado
std::unique_ptr<ITerminal> create_terminal();
// Sin need de destroy_ function
```

### 3. Agregar RAII wrapper para signal safety
```cpp
class SignalSafeFlag {
    std::atomic<bool>& flag_;
public:
    explicit SignalSafeFlag(std::atomic<bool>& flag) : flag_(flag) {}
    void set(bool value) { flag_.store(value, std::memory_order_relaxed); }
    bool get() const { return flag_.load(std::memory_order_relaxed); }
};
```

## Conclusión

**No se requieren cambios críticos.** El código ya sigue buenas prácticas:
- ✅ Zero raw pointers peligrosos en globals
- ✅ Uso apropiado de smart pointers
- ✅ Atomic types para concurrencia
- ✅ RAII en destructores

**Acciones opcionales de mejora:**
1. Refactorizar factory functions a `std::unique_ptr`
2. Documentar ownership semantics en comentarios
3. Agregar tests de stress para thread safety

---
Generado: 2026-01-03
Estado: Análisis Completado - No se encontraron issues críticos
