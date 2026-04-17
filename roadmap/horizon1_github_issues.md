# 📋 Horizonte 1 - Lista de Issues para GitHub

Este documento contiene el desglose del **Plan de Estabilización v0.3.x** en issues individuales listos para crear en GitHub Issue Tracker.

---

## 🏷️ Labels Sugeridos

Crear los siguientes labels en el repositorio:
- `horizon-1` (color: #0075ca)
- `optimization` (color: #ff9f1c)
- `tech-debt` (color: #fbca04)
- `security` (color: #d93f0b)
- `ci-cd` (color: #0075ca)
- `documentation` (color: #0075ca)
- `performance` (color: #ff9f1c)
- `good first issue` (color: #7057ff)

---

## 📦 Epic: Horizonte 1 - Estabilización v0.3.1

**Descripción:** Sprint de 2 semanas enfocado en optimización crítica, limpieza de arquitectura y robustez. No hay nuevas features, solo mejoras de calidad.

**Milestone:** `v0.3.1-stabilization`  
**Fecha Límite:** 2 semanas desde inicio  

---

## 🔧 Issues de Arquitectura (Día 1-2)

### Issue #1: [ARCH-01] Auditoría y eliminación de headers duplicados
**Label:** `horizon-1`, `tech-debt`, `good first issue`  
**Prioridad:** Alta  
**Assignee:** Por definir  

**Descripción:**
Identificar y eliminar headers duplicados entre `/src/` e `/include/`. Actualmente existen ~10-15 archivos que duplican definiciones, causando confusión y posibles inconsistencias.

**Tareas:**
- [ ] Ejecutar script para encontrar duplicados: `find . -name "*.h" -o -name "*.hpp" | xargs md5sum | sort | uniq -w32 -d`
- [ ] Listar todos los duplicados encontrados en un comment
- [ ] Decidir fuente de verdad para cada par (generalmente `/include/` es la correcta)
- [ ] Eliminar duplicados en `/src/`
- [ ] Actualizar todos los includes que referencien los archivos eliminados
- [ ] Verificar compilación exitosa en todas las plataformas

**Criterio de Aceptación:**
- ✅ 0 headers duplicados funcionales
- ✅ Compilación exitosa sin errores
- ✅ Tests unitarios pasando

**Recursos:**
- `/audit/CODE_QUALITY_AUDIT.md` (sección de hallazgos)

---

### Issue #2: [ARCH-02] Estandarización de estructura de includes
**Label:** `horizon-1`, `tech-debt`  
**Prioridad:** Alta  
**Assignee:** Por definir  

**Descripción:**
Establecer y aplicar convención clara para organización de headers:
- Headers públicos → `/include/desktop_tui/`
- Headers privados/internos → `/src/core/private/`

**Tareas:**
- [ ] Documentar convención en `/docs/architecture/include_structure.md`
- [ ] Mover headers existentes a ubicaciones correctas
- [ ] Actualizar CMakeLists.txt si es necesario para incluir directorios
- [ ] Revisar que headers privados no sean includibles desde fuera del repo

**Criterio de Aceptación:**
- ✅ Todos los headers en ubicaciones correctas
- ✅ Documentación actualizada
- ✅ Compilación exitosa

---

### Issue #3: [ARCH-03] Optimizar dependencias circulares y forward declarations
**Label:** `horizon-1`, `performance`, `tech-debt`  
**Prioridad:** Media  
**Assignee:** Por definir  

**Descripción:**
Revisar dependencias entre headers para reducir acoplamiento y tiempo de compilación. Usar forward declarations donde sea posible en lugar de includes completos.

**Tareas:**
- [ ] Usar `#pragma once` consistentemente en todos los headers
- [ ] Identificar includes innecesarios con herramienta `include-what-you-use`
- [ ] Reemplazar includes con forward declarations cuando solo se usan punteros/referencias
- [ ] Medir tiempo de compilación antes/después

**Criterio de Aceptación:**
- ✅ Tiempo de compilación reducido en ≥15%
- ✅ 0 warnings de include innecesario
- ✅ Todos los tests pasando

---

## ⚡ Issues de Rendimiento (Día 3-5)

### Issue #4: [PERF-01] Implementar stack buffer en Renderer::flush()
**Label:** `horizon-1`, `performance`, `optimization`  
**Prioridad:** Crítica  
**Assignee:** Developer senior  

**Descripción:**
El renderer actual usa `std::string buffer` que causa ~100K asignaciones de heap por segundo durante el render loop. Esto impacta severamente el rendimiento.

**Solución Propuesta:**
```cpp
// Reemplazar:
std::string buffer;
buffer.reserve(4096);

// Con:
std::array<char, 4096> stack_buffer;
size_t offset = 0;
// Escritura directa con bounds checking
```

**Tareas:**
- [ ] Profiling inicial para establecer línea base (usar `perf`, `valgrind --tool=massif`)
- [ ] Implementar stack buffer en `Renderer::flush()`
- [ ] Manejar caso edge donde output > 4096 bytes (fallback a heap o chunking)
- [ ] Benchmark post-implementación
- [ ] Validar que no haya regresiones visuales

**Criterio de Aceptación:**
- ✅ <10K allocs/sec en escenario de render estático (90% reducción)
- ✅ 0 regresiones de rendimiento en otros paths
- ✅ Tests visuales pasando

**Recursos:**
- `/benchmarks/render_benchmark.cpp`
- Herramientas: `heaptrack`, `valgrind massif`

---

### Issue #5: [PERF-02] Cambiar retornos por valor a referencias constantes
**Label:** `horizon-1`, `performance`, `optimization`  
**Prioridad:** Crítica  
**Assignee:** Developer senior  

**Descripción:**
Métodos como `Desktop::windows()`, `Window::widgets()` retornan contenedores por valor, causando copias profundas innecesarias.

**Cambios Requeridos:**
```cpp
// Antes:
std::vector<WindowPtr> windows() const;

// Después:
const std::vector<WindowPtr>& windows() const;
```

**Tareas:**
- [ ] Identificar todos los métodos que retornan contenedores por valor
- [ ] Cambiar signatures a retorno por referencia constante
- [ ] Documentar lifetime guarantees en comentarios (evitar dangling references)
- [ ] Actualizar código cliente que asuma ownership
- [ ] Benchmark de copias evitadas

**Criterio de Aceptación:**
- ✅ Reducción de copias en ≥95%
- ✅ 0 dangling references (validar con ASan)
- ✅ API documentation actualizada

**Archivos Afectados:**
- `/include/desktop_tui/desktop.h`
- `/include/desktop_tui/window.h`
- `/include/desktop_tui/widget.h`

---

### Issue #6: [PERF-03] Optimizar dirty regions con unión de rectángulos
**Label:** `horizon-1`, `performance`, `optimization`  
**Prioridad:** Media  
**Assignee:** Por definir  

**Descripción:**
El sistema actual de dirty regions no une rectángulos adyacentes, resultando en syscalls `write()` redundantes al terminal.

**Algoritmo Propuesto:**
```cpp
std::vector<Rect> union_adjacent(const std::vector<Rect>& regions) {
    // Unir rects que se tocan o solapan
    // Reducir N regiones a M regiones donde M < N
}
```

**Tareas:**
- [ ] Implementar algoritmo de unión de rectángulos
- [ ] Integrar en `Renderer::compute_dirty_regions()`
- [ ] Medir número de syscalls antes/después con `strace -c`
- [ ] Validar correctness visual

**Criterio de Aceptación:**
- ✅ Syscalls `write()` reducidos en ≥40%
- ✅ 0 regresiones visuales
- ✅ Overhead computacional del algoritmo <5% del frame time

---

### Issue #7: [PERF-04] Eliminar copias innecesarias en event dispatching
**Label:** `horizon-1`, `performance`, `optimization`  
**Prioridad:** Media  
**Assignee:** Por definir  

**Descripción:**
Los eventos se pasan por valor en varias capas del event dispatching, causando copias innecesarias de objetos `Event`.

**Tareas:**
- [ ] Auditar cadena completa de event handling
- [ ] Cambiar parámetros a `const Event&` donde aplique
- [ ] Usar `std::move` solo para transferencias de ownership reales
- [ ] Profiling para confirmar 0 copias innecesarias

**Criterio de Aceptación:**
- ✅ Profiling muestra 0 copias de `Event` en hot path
- ✅ Tests de eventos pasando

---

## 🛡️ Issues de Seguridad y Robustez (Día 6-7)

### Issue #8: [SEC-01] Habilitar sanitizers en build system
**Label:** `horizon-1`, `security`, `ci-cd`  
**Prioridad:** Alta  
**Assignee:** DevOps / Build engineer  

**Descripción:**
Configurar CMake para soportar builds con AddressSanitizer (ASan) y UndefinedBehaviorSanitizer (UBSan).

**Implementación:**
```cmake
option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
if(ENABLE_SANITIZERS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} 
        -fsanitize=address,undefined 
        -fno-omit-frame-pointer -g")
endif()
```

**Tareas:**
- [ ] Modificar CMakeLists.txt con opción `ENABLE_SANITIZERS`
- [ ] Crear preset `debug-sanitizer` en `CMakePresets.json`
- [ ] Documentar uso en `/docs/building.md`
- [ ] Validar build en Linux, macOS, Windows

**Criterio de Aceptación:**
- ✅ `cmake --preset debug-sanitizer` funciona
- ✅ Build exitoso en las 3 plataformas principales
- ✅ Documentación actualizada

---

### Issue #9: [SEC-02] Ejecución completa con ASan/UBSan y fix de issues
**Label:** `horizon-1`, `security`, `bug`  
**Prioridad:** Crítica  
**Assignee:** Developer senior  

**Descripción:**
Ejecutar todos los tests y demo app con sanitizers habilitados y fixear TODOS los reportes.

**Tareas:**
- [ ] Correr tests unitarios con ASan/UBSan
- [ ] Correr demo app por 5 minutos con interacción intensiva
- [ ] Catalogar todos los issues reportados
- [ ] Fixear cada issue (use-after-free, overflow, UB, etc.)
- [ ] Validar 0 errores restantes

**Criterio de Aceptación:**
- ✅ Salida limpia de sanitizers (0 errores)
- ✅ Todos los tests pasando
- ✅ Demo app estable por 10+ minutos

**Nota:** Asignar tiempo buffer de 2 días extra si se encuentran issues críticos.

---

### Issue #10: [SEC-03] Validación estricta de UTF-8
**Label:** `horizon-1`, `security`, `robustness`  
**Prioridad:** Media  
**Assignee:** Por definir  

**Descripción:**
Implementar validador de secuencias UTF-8 que rechace inputs mal formados en lugar de mostrar caracteres basura o corromper layout.

**Tareas:**
- [ ] Implementar `bool StringUtilities::is_valid_utf8(const std::string&)`
- [ ] Integrar validación en puntos de entrada de texto (inputs de usuario, clipboard, plugins)
- [ ] Definir comportamiento para UTF-8 inválido (rechazar con error, reemplazar con , etc.)
- [ ] Tests con casos edge (secuencias truncadas, overlong, invalid codepoints)

**Criterio de Aceptación:**
- ✅ Tests con UTF-8 inválido no crashan
- ✅ Error claro y controlado en lugar de corrupción
- ✅ 0 impacto en rendimiento para UTF-8 válido (<1% overhead)

---

### Issue #11: [SEC-04] Bounds checking en accesos a buffers
**Label:** `horizon-1`, `security`, `robustness`  
**Prioridad:** Alta  
**Assignee:** Por definir  

**Descripción:**
Revisar todos los accesos a arrays y raw pointers para asegurar bounds checking adecuado.

**Tareas:**
- [ ] Auditar código en busca de `operator[]` en raw arrays y `std::vector`
- [ ] Reemplazar con `.at()` que hace bounds checking, o agregar asserts explícitos
- [ ] En builds Debug, agregar `assert(index < size)` antes de accesos críticos
- [ ] Validar con ASan que no hay out-of-bounds accesses

**Criterio de Aceptación:**
- ✅ ASan no reporta out-of-bounds accesses
- ✅ 0 crashes por accesos inválidos en testing intensivo

---

## 📝 Issues de Logging (Día 8)

### Issue #12: [LOG-01] Diseñar e implementar sistema de logging unificado
**Label:** `horizon-1`, `feature`, `developer-experience`  
**Prioridad:** Media  
**Assignee:** Por definir  

**Descripción:**
Crear sistema de logging ligero, zero-dependency, con niveles configurables y múltiples sinks.

**Requerimientos:**
- Niveles: TRACE, DEBUG, INFO, WARN, ERROR, FATAL
- Sinks: consola (stderr), archivo opcional
- Thread-safe
- Macro-based para capturar file/line automáticamente
- Overhead <1% en builds Release

**API Propuesta:**
```cpp
DT_LOG(INFO) << "Window created: " << window_id;
DT_LOG_DEBUG("Event dispatched", event_type);
DT_LOG_ERROR << "Failed to load plugin: " << error_msg;
```

**Tareas:**
- [ ] Diseñar API en `/docs/logging.md`
- [ ] Implementar logger en `/src/core/logger.{h,cpp}`
- [ ] Implementar macros `DT_LOG_*`
- [ ] Configurar niveles via variable de entorno `DESKTOP_TUI_LOG_LEVEL`
- [ ] Tests de funcionalidad básica

**Criterio de Aceptación:**
- ✅ Logger funcional en todos los niveles
- ✅ Overhead medido <1% en release
- ✅ Documentación completa

---

### Issue #13: [LOG-02] Integrar logging en componentes críticos
**Label:** `horizon-1`, `feature`  
**Prioridad:** Baja  
**Assignee:** Por definir  

**Descripción:**
Añadir logs estratégicos en componentes clave para facilitar debugging en producción.

**Puntos de Logging:**
- Creación/destrucción de ventanas
- Dispatch de eventos (solo nivel DEBUG)
- Flush de renderer (warnings si toma >16ms)
- Carga/fallo de plugins
- Errores de plataforma (terminal, input)

**Tareas:**
- [ ] Añadir logs en Desktop, Window, Renderer, PluginManager
- [ ] Validar que logs son útiles pero no verbosos en INFO
- [ ] Configurar nivel DEBUG para desarrollo

**Criterio de Aceptación:**
- ✅ Logs informativos sin spam
- ✅ Útil para troubleshooting real

---

## 🔄 Issues de CI/CD (Día 9)

### Issue #14: [CI-01] Configurar GitHub Actions matrix build
**Label:** `horizon-1`, `ci-cd`  
**Prioridad:** Alta  
**Assignee:** DevOps engineer  

**Descripción:**
Crear workflow de GitHub Actions que compile y teste en múltiples plataformas y configuraciones.

**Matriz de Builds:**
- **Ubuntu:** gcc-11, gcc-13
- **macOS:** clang (Xcode)
- **Windows:** MSVC 2019, clang-cl

**Configuraciones:** Debug, Release

**Tareas:**
- [ ] Crear `.github/workflows/ci.yml`
- [ ] Configurar matrix strategy
- [ ] Instalar dependencias necesarias en cada runner
- [ ] Ejecutar tests unitarios en cada build
- [ ] Subir artifacts (binarios de demo) para Release builds

**Criterio de Aceptación:**
- ✅ Workflow ejecuta exitosamente en todas las combinaciones
- ✅ Tests pasan en todas las plataformas
- ✅ Build time <30 minutos total

---

### Issue #15: [CI-02] Integrar sanitizers en pipeline CI
**Label:** `horizon-1`, `ci-cd`, `security`  
**Prioridad:** Alta  
**Assignee:** DevOps engineer  

**Descripción:**
Añadir job dedicado que compile con ASan/UBSan y falle automáticamente si hay errores de memoria.

**Tareas:**
- [ ] Añadir job `sanitizer-check` en workflow CI
- [ ] Compilar con `-fsanitize=address,undefined`
- [ ] Correr todos los tests
- [ ] Fail job si hay algún reporte de sanitizer
- [ ] Configurar timeout apropiado (tests con ASan son más lentos)

**Criterio de Aceptación:**
- ✅ Job falla automáticamente ante memory errors
- ✅ Reports de ASan se muestran en logs de CI
- ✅ No falsos positivos

---

### Issue #16: [CI-03] Implementar cache de build para acelerar CI
**Label:** `horizon-1`, `ci-cd`, `performance`  
**Prioridad:** Media  
**Assignee:** DevOps engineer  

**Descripción:**
Usar `actions/cache` para cachear dependencias y objetos compilados, reduciendo tiempos de CI en ≥50%.

**Tareas:**
- [ ] Configurar cache para ccache (compilación incremental)
- [ ] Cache de dependencias del sistema (si aplica)
- [ ] Medir tiempos antes/después
- [ ] Ajustar key de cache para invalidación apropiada

**Criterio de Aceptación:**
- ✅ Tiempos de CI reducidos en ≥50% en builds subsiguientes
- ✅ Cache hit rate >80%

---

### Issue #17: [CI-04] Añadir badges de estado al README
**Label:** `horizon-1`, `ci-cd`, `documentation`  
**Prioridad:** Baja  
**Assignee:** Por definir  

**Descripción:**
Añadir badges visuales en README.md mostrando estado de builds, tests, y otras métricas.

**Badges Sugeridos:**
- Build status (GitHub Actions)
- Platforms supported (Linux, macOS, Windows)
- License
- Release version

**Tareas:**
- [ ] Generar URLs de badges de GitHub Actions
- [ ] Actualizar README.md con badges en sección superior
- [ ] Validar que badges se renderizan correctamente

**Criterio de Aceptación:**
- ✅ README actualizado con badges funcionales
- ✅ Badges reflejan estado real del repo

---

## 📚 Issues de Documentación (Día 10)

### Issue #18: [DOC-01] Documentar API pública de clases core
**Label:** `horizon-1`, `documentation`  
**Prioridad:** Alta  
**Assignee:** Technical writer / Developer  

**Descripción:**
Crear documentación completa de API para clases principales: Desktop, Window, Widget, Renderer, Event.

**Contenido Requerido:**
- Descripción de cada clase
- Métodos públicos con parámetros y return values
- Ejemplos de uso básico
- Notas de thread-safety
- Lifetime guarantees (especialmente para referencias)

**Formato:** Markdown en `/docs/api/core_classes.md`

**Tareas:**
- [ ] Documentar clase `Desktop`
- [ ] Documentar clase `Window`
- [ ] Documentar clase `Widget` y hierarchy
- [ ] Documentar clase `Renderer`
- [ ] Documentar clase `Event` y tipos
- [ ] Añadir ejemplos de código

**Criterio de Aceptación:**
- ✅ Usuario nuevo puede entender API sin leer headers
- ✅ Ejemplos son copy-pasteables y funcionales
- ✅ Todas las preguntas comunes respondidas

---

### Issue #19: [DOC-02] Actualizar CHANGELOG para v0.3.1
**Label:** `horizon-1`, `documentation`  
**Prioridad:** Media  
**Assignee:** Release manager  

**Descripción:**
Actualizar CHANGELOG.md siguiendo formato [Keep a Changelog](https://keepachangelog.com/) con todos los cambios de v0.3.1.

**Secciones:**
- Added (nuevas features, si alguna)
- Changed (cambios en funcionalidad existente)
- Deprecated (próximamente removido)
- Removed (features eliminados)
- Fixed (bugs corregidos)
- Security (vulnerabilidades corregidas)
- Performance (optimizaciones)

**Tareas:**
- [ ] Recopilar todos los commits desde v0.3.0
- [ ] Categorizar cambios según formato
- [ ] Escribir descripciones claras y concisas
- [ ] Incluir links a issues/PRs relevantes
- [ ] Fecha de release

**Criterio de Aceptación:**
- ✅ CHANGELOG.md actualizado
- ✅ Formato consistente con releases anteriores
- ✅ Links válidos

---

### Issue #20: [DOC-03] Crear guía de migración (si hay breaking changes)
**Label:** `horizon-1`, `documentation`  
**Prioridad:** Media (condicional)  
**Assignee:** Technical writer  

**Descripción:**
Si hubo cambios breaking en API pública (ej. signatures de métodos, comportamientos), crear guía paso a paso para migrar desde v0.3.0 a v0.3.1.

**Contenido:**
- Lista de breaking changes
- Código antes/después para cada cambio
- Script o checklist de migración
- FAQ de problemas comunes

**Tareas:**
- [ ] Identificar breaking changes (si los hay)
- [ ] Escribir guía en `/docs/migration/v0.3.0-to-v0.3.1.md`
- [ ] Proveer ejemplos concretos

**Criterio de Aceptación:**
- ✅ Usuarios existentes pueden actualizar sin sorpresas
- ✅ Guía clara y accionable

**Nota:** Si no hay breaking changes, marcar este issue como "Not required".

---

### Issue #21: [REL-01] Preparar release v0.3.1 (versionado y tagging)
**Label:** `horizon-1`, `release`  
**Prioridad:** Alta  
**Assignee:** Release manager  

**Descripción:**
Proceso final de release: actualizar versión, crear tag git, publicar release notes.

**Tareas:**
- [ ] Actualizar versión en `CMakeLists.txt` (PROJECT_VERSION)
- [ ] Actualizar versión en `include/desktop_tui/version.h`
- [ ] Commit de cambios de versión
- [ ] Crear tag git: `git tag -a v0.3.1 -m "Release v0.3.1 - Stabilization"`
- [ ] Push tag: `git push origin v0.3.1`
- [ ] Crear GitHub Release con release notes desde CHANGELOG
- [ ] Anunciar release en canales apropiados (Discord, Twitter, etc.)

**Criterio de Aceptación:**
- ✅ Tag v0.3.1 creado y publicado
- ✅ GitHub Release con notes completas
- ✅ Binarios disponibles (si aplica)

---

## 📊 Tracking de Progreso

### Dashboard de Issues

| Estado | Cantidad | Issues |
|--------|----------|--------|
| **Open** | 21 | Todos los listados arriba |
| **In Progress** | 0 | - |
| **Review** | 0 | - |
| **Done** | 0 | - |

### Milestone Completion

- [ ] Architecture cleanup (3/3 issues)
- [ ] Performance optimization (4/4 issues)
- [ ] Security & robustness (4/4 issues)
- [ ] Logging system (2/2 issues)
- [ ] CI/CD pipeline (4/4 issues)
- [ ] Documentation (4/4 issues)

**Total:** 0/21 issues completed

---

## 🎯 Siguiente Paso

1. **Crear milestone** `v0.3.1-stabilization` en GitHub
2. **Crear todos los issues** listados arriba y asignar al milestone
3. **Asignar developers** a issues según disponibilidad y expertise
4. **Iniciar sprint** con kickoff meeting

---

*Documento generado automáticamente desde `/roadmap/horizon1_stabilization_plan.md`*
