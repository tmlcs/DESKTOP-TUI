# 🛡️ Plan Detallado: Fase 1 - Seguridad Crítica
**Versión:** Desktop TUI v0.3.0 → v0.4.0 (Hardening Release)  
**Duración Estimada:** 2 Semanas  
**Prioridad:** CRÍTICA (Bloqueante para v1.0)  
**Responsable:** Core Team

---

## 📋 Resumen Ejecutivo

Esta fase aborda **4 vulnerabilidades de seguridad críticas** identificadas en la auditoría de código. La implementación de estos fixes es obligatoria antes de considerar cualquier release público o integración en producción. El objetivo es eliminar vectores de ataque por desbordamiento de buffer, condiciones de carrera en señales UNIX y gestión insegura de recursos.

### Métricas de Éxito
- ✅ 0 vulnerabilidades críticas reportadas por sanitizers (ASan, UBSan, TSan).
- ✅ 100% de los tests existentes pasando + 15 nuevos tests de seguridad.
- ✅ Cobertura de código en módulos críticos > 95%.
- ✅ Documentación de seguridad actualizada.

---

## 🔴 Issue #1: Posible Crecimiento Ilimitado del Buffer de Entrada

### Descripción
El buffer de entrada (`InputBuffer` en `src/platform/IInput.hpp`) no tiene un límite estricto de caracteres acumulados. Un atacante podría enviar una secuencia masiva de bytes (ej. mediante script o dispositivo virtual) causando:
- Consumo excesivo de memoria (DoS).
- Desbordamiento de entero al calcular tamaños.
- Lentitud extrema en el procesamiento de eventos.

### Solución Técnica
1. **Definir Constante Global:** `MAX_INPUT_BUFFER_SIZE` (sugerido: 64KB o 1024 caracteres pendientes).
2. **Modificar `IInput::readEvent()`:**
   - Verificar tamaño actual del buffer antes de agregar nuevos datos.
   - Si se excede el límite: descartar eventos antiguos (política circular) o ignorar nuevos hasta vaciar.
   - Loguear advertencia de seguridad si se alcanza el 80% de capacidad.
3. **Refuerzo en `PosixInput` y `WinInput`:** Asegurar que los buffers internos de lectura (`char buf[256]`) no se acumulen sin control en el vector principal.

### Archivos a Modificar
- `src/platform/IInput.hpp` (Interfaz y constantes)
- `src/platform/PosixInput.cpp` (Implementación Linux/macOS)
- `src/platform/WinInput.cpp` (Implementación Windows)
- `tests/unit/InputBufferTests.cpp` (Nuevos tests de estrés)

### Criterios de Aceptación
- [ ] El buffer nunca supera `MAX_INPUT_BUFFER_SIZE`.
- [ ] Test unitario que inyecta 1MB de datos y verifica que el sistema no colapsa ni usa >64KB.
- [ ] No hay regresión en latencia de input normal (<1ms overhead).

---

## 🔴 Issue #2: Validación Faltante en Dimensiones Negativas para Resize

### Descripción
Las señales `SIGWINCH` (Linux/macOS) o eventos de consola (Windows) pueden reportar dimensiones erróneas o maliciosas (negativas o cero) en entornos emulados o corruptos.
- `Renderer::resize(int w, int h)` asume `w > 0` y `h > 0`.
- Valores negativos causan:
  - Asignación de memoria gigantesca (underflow en cálculos de tamaño).
  - Crash inmediato por `std::bad_alloc` o acceso inválido.

### Solución Técnica
1. **Validación Defensiva en `Terminal::resize()`:**
   ```cpp
   if (w <= 0 || h <= 0 || w > MAX_TERM_WIDTH || h > MAX_TERM_HEIGHT) {
       Logger::warn("Resize ignorado: dimensiones inválidas {}x{}", w, h);
       return; // Mantener tamaño anterior
   }
   ```
2. **Constantes de Límite:** Definir `MAX_TERM_WIDTH` y `MAX_TERM_HEIGHT` (ej. 10000x10000) para evitar allocaciones absurdas aunque sean positivas.
3. **Sanitización en Signal Handler:** El handler de `SIGWINCH` debe leer `winsize`, validar y *solo entonces* llamar a la lógica de resize.

### Archivos a Modificar
- `src/core/Terminal.hpp` / `.cpp`
- `src/platform/PosixTerminal.cpp`
- `src/platform/WinTerminal.cpp`
- `tests/unit/RendererTests.cpp`

### Criterios de Aceptación
- [ ] Enviar señal con dimensiones `-1x-1` no crashea el programa.
- [ ] Enviar dimensiones `99999x99999` no causa OOM (Out Of Memory).
- [ ] El terminal mantiene su tamaño válido anterior ante fallo.

---

## 🔴 Issue #3: Puntero Colgante `g_active_terminal`

### Descripción
Existe una variable global `g_active_terminal` (o similar patrón singleton crudo) usada en handlers de señales o callbacks estáticos.
- Si el objeto `Terminal` se destruye (ej. shutdown parcial) y llega una señal tardía, el puntero queda colgante (dangling pointer).
- Resultado: **Segmentation Fault** determinista o corrupción de memoria aleatoria.

### Solución Técnica
1. **Reemplazar con `std::weak_ptr`:**
   - Cambiar `static Terminal* g_active_terminal` por `static std::weak_ptr<Terminal> g_active_terminal_weak`.
   - En el handler: intentar hacer `lock()`. Si falla (nullptr), ignorar la señal silenciosamente.
2. **Alternativa (si no hay smart pointers):** Usar bandera atómica `g_is_terminal_valid`.
   - Setear `true` en constructor, `false` en destructor.
   - Chequear bandera antes de dereferenciar puntero global.
3. **Garantizar Orden de Destrucción:** Asegurar que el registro de señales se limpie *antes* de destruir el objeto Terminal (`signal(SIGWINCH, SIG_DFL)`).

### Archivos a Modificar
- `src/core/Terminal.cpp` (Gestión del singleton/puntero)
- `src/main.cpp` (Inicialización y cleanup de señales)
- `src/platform/SignalHandler.cpp` (Si existe módulo separado)

### Criterios de Aceptación
- [ ] Test que destruye el Terminal y envía señal artificialmente: no debe crashear.
- [ ] Análisis estático (Clang Static Analyzer) no reporta "Dereference of null/garbage pointer".
- [ ] Uso de `std::atomic` para banderas de estado compartidas.

---

## 🔴 Issue #4: Manejo de Señales Antes de Inicialización Terminal

### Descripción
Si una señal (`SIGINT`, `SIGWINCH`) llega *inmediatamente* después del inicio del programa, antes de que `Terminal::init()` complete la configuración del modo raw o la asignación de buffers:
- El handler intenta acceder a recursos no inicializados.
- Puede causar corrupción de estado o crash en los primeros milisegundos de vida.

### Solución Técnica
1. **Bandera Atómica de Estado:** `std::atomic<bool> g_is_initialized{false}`.
   - Setear a `true` *al final* de `Terminal::init()`.
   - Setear a `false` al inicio de `Terminal::shutdown()`.
2. **Guard en Handlers:**
   ```cpp
   void sigwinch_handler(int) {
       if (!g_is_initialized.load()) return;
       // Lógica segura...
   }
   ```
3. **Bloqueo Temporal (Opcional):** En sistemas POSIX, usar `sigprocmask` para bloquear señales durante la inicialización crítica (sección de 50-100 líneas en `main()`).

### Archivos a Modificar
- `src/main.cpp` (Flujo de inicio)
- `src/core/Terminal.cpp` (Manejo de estado interno)
- `src/platform/SignalHandler.cpp`

### Criterios de Aceptación
- [ ] Script que envía `SIGWINCH` inmediatamente tras lanzar el binario no produce crash.
- [ ] Code review confirma que ningún recurso global se toca antes de `g_is_initialized = true`.

---

## 📅 Cronograma de Implementación (2 Semanas)

| Día | Actividad | Entregable |
| :--- | :--- | :--- |
| **Día 1** | Análisis profundo de `IInput` y diseño de límite de buffer. | Documento de diseño técnico. |
| **Día 2-3** | Implementación Fix #1 (Input Buffer Limit) + Tests. | PR #101 merged. |
| **Día 4** | Análisis de flujo de `resize` y señales. | Checklist de validación. |
| **Día 5-6** | Implementación Fix #2 (Dimensiones Negativas) + Tests. | PR #102 merged. |
| **Día 7** | Revisión de arquitectura Singleton/Punteros globales. | Diagrama de vida de objetos. |
| **Día 8-9** | Implementación Fix #3 (Puntero Colgante) + Tests. | PR #103 merged. |
| **Día 10** | Auditoría de secuencia de inicio/shutdown. | Reporte de puntos de fallo. |
| **Día 11-12**| Implementación Fix #4 (Señales Tempranas) + Tests. | PR #104 merged. |
| **Día 13** | Ejecución de Sanitizers (ASan, UBSan, TSan) en CI local. | Reporte limpio de sanitizers. |
| **Día 14** | Code Review final, merge a `develop` y tag v0.4.0-alpha. | Release Notes v0.4.0. |

---

## 🧪 Estrategia de Testing

### 1. Tests Unitarios Nuevos (`tests/unit/SecurityTests.cpp`)
- `TEST(InputBuffer, RejectsMassiveInput)`: Inyecta 1MB, verifica límite.
- `TEST(Terminal, HandlesNegativeResize)`: Fuerza resize(-10, -10), verifica estabilidad.
- `TEST(Terminal, HandlesHugeResize)`: Fuerza resize(99999, 99999), verifica no-alloc.
- `TEST(Signal, SafeAfterDestruction)`: Destruye terminal, envía señal, verifica no-crash.
- `TEST(Signal, SafeBeforeInit)`: Envía señal antes de `init()`, verifica ignorado.

### 2. Tests de Estrés (Fuzzing Ligero)
- Script Python que envía secuencias aleatorias de bytes por stdin a máxima velocidad durante 60s.
- Herramienta `kill -SIGWINCH <pid>` en bucle rápido mientras se redimensiona ventana real.

### 3. Sanitizers
Compilar con flags:
```bash
g++ -fsanitize=address,undefined -g ...
clang++ -fsanitize=thread -g ...
```
Ejecutar suite completa. Cualquier reporte es bloqueo para merge.

---

## 🚀 Impacto Esperado

- **Seguridad:** Eliminación del 100% de vulnerabilidades críticas conocidas.
- **Estabilidad:** Reducción del 90% en crashes por condiciones de borde externas.
- **Confianza:** Base sólida para implementar features complejas (red, plugins) en fases siguientes sin miedo a colapsos básicos.
- **Reputación:** Demostración de compromiso con seguridad desde el día 1, crucial para adopción empresarial.

---

## ⚠️ Riesgos y Mitigación

| Riesgo | Probabilidad | Impacto | Mitigación |
| :--- | :--- | :--- | :--- |
| Regresión en performance de input | Media | Medio | Benchmarking antes/después. Ajustar `MAX_INPUT_BUFFER_SIZE` si es necesario. |
| Complejidad en manejo de `weak_ptr` | Baja | Alto | Pair programming en la implementación. Revisión por experto en concurrencia. |
| Dificultad para reproducir race conditions | Alta | Medio | Uso intensivo de TSan y pruebas repetitivas en CI (loop x100). |

---

## ✅ Definition of Done (DoD) para Fase 1

1. [ ] Los 4 fixes están implementados y mergados en `develop`.
2. [ ] Todos los tests unitarios pasan (locales y CI).
3. [ ] Ejecución limpia con AddressSanitizer y UndefinedBehaviorSanitizer.
4. [ ] Documentación de API actualizada (si hubo cambios públicos).
5. [ ] Changelog generado para v0.4.0.
6. [ ] Aprobación formal del Tech Lead en Pull Request.

---

**Nota Final:** Esta fase no añade funcionalidad visible al usuario final, pero es el cimiento indispensable para que Desktop TUI sea considerado software de grado productivo. **No saltar bajo ninguna circunstancia.**
