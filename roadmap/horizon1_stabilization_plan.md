# 🎯 Plan Detallado: Horizonte 1 - Estabilización v0.3.x

**Duración:** 2 semanas (10 días laborables)  
**Versión Objetivo:** v0.3.1 (Patch Release)  
**Estado Actual:** v0.3.0  
**Responsable:** Equipo de Desarrollo  

---

## 📋 Objetivos del Horizonte 1

Este sprint se enfoca exclusivamente en **estabilización, optimización crítica y calidad de código**. No se añadirán features nuevas. El objetivo es preparar una base sólida y performante para las features de v0.4.0.

### Metas Cuantificables
- [ ] Reducir asignaciones de heap en render loop en ≥90%
- [ ] 0 warnings en AddressSanitizer (ASan) y UndefinedBehaviorSanitizer (UBSan)
- [ ] Eliminar 100% de headers duplicados
- [ ] Documentar API pública de componentes core
- [ ] Configurar pipeline CI/CD con sanitizers habilitados

---

## 🗓️ Sprint Breakdown (10 Días)

### **Día 1-2: Limpieza de Arquitectura y Headers**
**Objetivo:** Eliminar deuda técnica y duplicación de código.

#### Tareas:
1. **[ARCH-01] Auditoría de Headers**
   - Identificar todos los headers en `/src/` que duplican `/include/`
   - Decidir fuente de verdad para cada header duplicado
   - Eliminar duplicados y actualizar includes en todo el proyecto
   - **Criterio de Aceptación:** `find /workspace -name "*.h" -o -name "*.hpp"` no muestra duplicados funcionales

2. **[ARCH-02] Estandarización de Includes**
   - Crear directriz: "Headers públicos en `/include/desktop_tui/`, privados en `/src/core/private/`"
   - Mover headers según directriz
   - Actualizar CMakeLists.txt si es necesario
   - **Criterio de Aceptación:** Compilación exitosa sin errores de include

3. **[ARCH-03] Refactorización de Dependencias Circulares**
   - Usar `#pragma once` consistentemente
   - Reemplazar forward declarations donde sea posible para reducir acoplamiento
   - **Criterio de Aceptación:** Tiempo de compilación reducido en ≥15%

**Entregable:** Árbol de directorios limpio, 0 duplicados.

---

### **Día 3-5: Optimización Crítica de Rendimiento**
**Objetivo:** Atacar los hot paths identificados en la auditoría.

#### Tareas:
1. **[PERF-01] Stack Buffer en Renderer::flush()**
   - **Problema:** `std::string buffer` causa ~100K allocs/sec
   - **Solución:** 
     ```cpp
     // Antes:
     std::string buffer;
     buffer.reserve(4096);
     
     // Después:
     std::array<char, 4096> stack_buffer;
     size_t offset = 0;
     // Uso de snprintf o escritura directa con bounds checking
     ```
   - **Benchmark:** Medir allocs/sec antes/después con `heaptrack` o `valgrind --tool=massif`
   - **Criterio de Aceptación:** <10K allocs/sec en escenario de render estático

2. **[PERF-02] Retorno por Referencia en Contenedores**
   - **Identificar:** `Desktop::windows()`, `Window::widgets()`, métodos similares
   - **Refactorizar:**
     ```cpp
     // Antes:
     std::vector<WindowPtr> windows() const; // Retorna por valor (copia profunda)
     
     // Después:
     const std::vector<WindowPtr>& windows() const; // Referencia constante
     ```
   - **Cuidado:** Documentar lifetime del reference para evitar dangling references
   - **Criterio de Aceptación:** Tests de rendimiento muestran reducción de copias en 95%

3. **[PERF-03] Dirty Regions Optimization**
   - **Mejora:** Unir rectángulos adyacentes antes de renderizar
   - **Algoritmo:** Implementar unión de rects solapados/tocantes
     ```cpp
     std::vector<Rect> union_adjacent(const std::vector<Rect>& regions);
     ```
   - **Criterio de Aceptación:** Número de syscalls `write()` reducido en ≥40%

4. **[PERF-04] Evitar Copias en Event Dispatching**
   - Pasar `Event` por `const&` en toda la cadena de handling
   - Usar `std::move` solo cuando sea necesario transferir ownership
   - **Criterio de Aceptación:** Profiling muestra 0 copias innecesarias de `Event`

**Entregable:** Benchmarks comparativos pre/post optimización, reducción drástica de allocs.

---

### **Día 6-7: Robustez y Sanitizers**
**Objetivo:** Garantizar seguridad de memoria y comportamiento definido.

#### Tareas:
1. **[SEC-01] Habilitar Sanitizers en Build System**
   - Modificar CMakeLists.txt:
     ```cmake
     option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
     if(ENABLE_SANITIZERS)
         set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer -g")
     endif()
     ```
   - Crear preset `Debug-Sanitizer` en `CMakePresets.json`
   - **Criterio de Aceptación:** `cmake --preset debug-sanitizer && cmake --build` funciona

2. **[SEC-02] Ejecución Completa con ASan/UBSan**
   - Correr todos los tests unitarios con sanitizers
   - Correr demo app por 5 minutos interactuando intensivamente
   - Fixear TODOS los reportes (use-after-free, overflow, undefined behavior)
   - **Criterio de Aceptación:** Salida limpia de sanitizers, 0 errores

3. **[SEC-03] Validación UTF-8 Estricta**
   - Implementar validador de secuencias UTF-8 en `StringUtilities`
   - Rechazar inputs mal formados con error claro en lugar de corrupción
   - **Criterio de Aceptación:** Tests con UTF-8 inválido no crashan, muestran error controlado

4. **[SEC-04] Bounds Checking en Acceso a Buffers**
   - Revisar todos los accesos a arrays/raw pointers
   - Reemplazar con `.at()` o agregar asserts en builds Debug
   - **Criterio de Aceptación:** ASan no reporta out-of-bounds accesses

**Entregable:** Builds con sanitizers limpios, documentación de flags de build.

---

### **Día 8: Sistema de Logs Unificado**
**Objetivo:** Mejorar debuggabilidad y diagnóstico en producción.

#### Tareas:
1. **[LOG-01] Diseñar API de Logging**
   - Niveles: `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`
   - Soporte para múltiples sinks (consola, archivo, stderr)
   - Thread-safe (usando mutex o lock-free queue si es crítico)
   - **Criterio de Aceptación:** API documentada en `/docs/logging.md`

2. **[LOG-02] Implementar Logger Ligero**
   - Zero dependencies, solo C++17
   - Macro-based para capturar file/line automáticamente:
     ```cpp
     DT_LOG(INFO) << "Window created: " << window_id;
     DT_LOG_DEBUG("Event dispatched", event_type);
     ```
   - **Criterio de Aceptación:** Logger funcional, overhead <1% en release

3. **[LOG-03] Integrar en Componentes Críticos**
   - Añadir logs en: creación/destrucción de ventanas, dispatch de eventos, flush de renderer
   - **Criterio de Aceptación:** Logs útiles para debugging sin ser verbosos en release

**Entregable:** Sistema de logs funcional, ejemplos de uso.

---

### **Día 9: CI/CD Pipeline Robusto**
**Objetivo:** Automatizar calidad y prevenir regresiones.

#### Tareas:
1. **[CI-01] Configurar GitHub Actions Matrix**
   - Matriz: Ubuntu (gcc-11, gcc-13), macOS (clang), Windows (MSVC, clang-cl)
   - Builds: Debug, Release, Debug-Sanitizer
   - **Criterio de Aceptación:** Workflow `.github/workflows/ci.yml` funcional

2. **[CI-02] Integrar Sanitizers en CI**
   - Job dedicado que compile con ASan/UBSan y corra tests
   - Fail rápido si hay errores de memoria
   - **Criterio de Aceptación:** CI falla automáticamente ante memory errors

3. **[CI-03] Cache de Dependencias y Build**
   - Usar `actions/cache` para acelerar builds (ccache, vcpkg si aplica)
   - **Criterio de Aceptación:** Tiempos de CI reducidos en ≥50%

4. **[CI-04] Badge de Estado en README**
   - Añadir badges de build status, coverage (si aplica)
   - **Criterio de Aceptación:** README actualizado con badges

**Entregable:** Pipeline CI/CD verde en todas las plataformas.

---

### **Día 10: Documentación y Release Prep**
**Objetivo:** Preparar release v0.3.1 con documentación clara.

#### Tareas:
1. **[DOC-01] Documentar API Pública Core**
   - Crear `/docs/api/core_classes.md` con:
     - `Desktop`, `Window`, `Widget`, `Renderer`, `Event`
     - Métodos públicos, parámetros, return values, thread-safety
   - **Criterio de Aceptación:** Usuario puede entender API sin leer headers

2. **[DOC-02] CHANGELOG v0.3.1**
   - Listar todos los cambios, fixes, optimizations
   - Formato: [Keep a Changelog](https://keepachangelog.com/)
   - **Criterio de Aceptación:** CHANGELOG.md actualizado

3. **[DOC-03] Guía de Migración (si hay breaking changes)**
   - Si hubo cambios en signatures de funciones públicas, documentar cómo migrar
   - **Criterio de Aceptación:** Usuarios existentes pueden actualizar sin sorpresas

4. **[REL-01] Versionado y Tagging**
   - Actualizar versión en `CMakeLists.txt` y `version.h`
   - Crear tag git `v0.3.1`
   - **Criterio de Aceptación:** Tag creado, release notes publicadas

**Entregable:** Release v0.3.1 listo para publicar.

---

## 📊 Métricas de Éxito del Horizonte 1

| Métrica | Línea Base (v0.3.0) | Objetivo (v0.3.1) | Cómo Medir |
|---------|---------------------|-------------------|------------|
| **Allocs/sec (idle)** | ~100,000 | <10,000 | `heaptrack`, `valgrind massif` |
| **Copias de contenedores** | Altas (por valor) | <5% de ops | Profiling con perf/VTune |
| **Warnings ASan/UBSan** | Desconocido | 0 | Ejecución con sanitizers |
| **Headers duplicados** | ~10-15 archivos | 0 | Script de auditoría |
| **Tiempo de compilación** | X segundos | X - 15% | `time make` |
| **Syscalls write()** | N por frame | N - 40% | `strace -c` |
| **Cobertura de Tests** | 13.3% | Mantener o mejorar | `gcov`/`lcov` |
| **Builds en CI** | Limitado | 6 combinaciones | GitHub Actions matrix |

---

## 🛠️ Recursos y Herramientas Necesarias

### Herramientas de Profiling
- **Linux:** `valgrind` (memcheck, massif), `perf`, `heaptrack`
- **macOS:** Instruments (Time Profiler, Allocations)
- **Windows:** Visual Studio Profiler, ETW

### Herramientas de Build
- CMake ≥3.20
- GCC 11+, Clang 14+, MSVC 2019+
- ccache (para acelerar recompilaciones)

### CI/CD
- GitHub Actions (gratis para repos públicos)
- Opción alternativa: GitLab CI, Azure Pipelines

---

## ⚠️ Riesgos y Mitigación

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|--------------|---------|------------|
| Breaking changes en API pública | Media | Alto | Versionado semántico, guía de migración clara |
| Optimizaciones introducen bugs | Media | Medio | Tests exhaustivos post-cambio, rollback plan |
| CI se vuelve lento (>30 min) | Baja | Medio | Parallel jobs, cache estratégico |
| Sanitizers encuentran issues críticos | Alta | Alto | Asignar tiempo buffer (2 días extra) para fixes |

---

## 📝 Checklist de Definición de "Terminado" (DoD)

Para cada tarea del sprint, debe cumplirse:
- [ ] Código implementado y compilando sin warnings
- [ ] Tests unitarios pasando (locales y en CI)
- [ ] Profiling/benchmark realizado (si aplica)
- [ ] Documentación actualizada (si cambia API pública)
- [ ] Code review aprobado (al menos 1 reviewer)
- [ ] Sanitizers limpios en la rama

---

## 🚀 Próximos Pasos Inmediatos (Post-Horizonte 1)

Una vez completado v0.3.1, el equipo procederá al **Horizonte 2: Ecosistema v0.4.x**:
1. Sistema de temas JSON recargables
2. Widgets avanzados (Grid, Table, TreeView)
3. Motor de plugins con sandboxing básico
4. Integration tests visuales

---

## 📞 Comunicación y Seguimiento

- **Daily Standup:** 15 mins diarios (async en Slack/Discord o sync)
- **Mid-Sprint Review:** Día 5 (revisar progreso de optimizaciones)
- **Sprint Review:** Día 10 (demo de v0.3.1, métricas finales)
- **Herramienta de Tracking:** GitHub Projects o Issue Tracker con labels `horizon-1`, `optimization`, `tech-debt`

---

**Aprobado por:** ___________________  
**Fecha de Inicio:** _________________  
**Fecha de Fin Estimada:** ___________

---

*Documento vivo: Actualizar conforme avanza el sprint.*
