# 🧠 Sesión de Ideación: Desktop TUI Framework v0.3.0+

**Fecha:** Abril 2026  
**Versión Actual:** v0.3.0  
**Basado en:** Auditoría de Calidad Detallada (Score: B+, 84/100)  
**Objetivo:** Definir roadmap estratégico para v0.3.1 → v0.4.0 → v1.0.0

---

## 📊 Estado Actual del Proyecto

### Métricas Clave
- **Líneas de Código:** ~9,458 LOC (C++17 puro)
- **Archivos:** 58 archivos fuente
- **Tests:** 15 tests unitarios + benchmarks
- **Cobertura:** ~33% (objetivo: >80%)
- **Dependencias:** Zero (solo STL + APIs plataforma)
- **Plataformas:** Linux ✅, macOS ✅, Windows ✅, Android/Termux ⚠️

### Fortalezas Identificadas (Auditoría)
✅ Arquitectura por capas bien definida (90/100)  
✅ Seguridad robusta sin vulnerabilidades críticas (88/100)  
✅ Optimizaciones avanzadas (dirty-region rendering, zero-allocation flush)  
✅ Sistema de eventos publish/subscribe elegante  
✅ UTF-8 completo con soporte CJK wide characters  
✅ TrueColor (24-bit) + 256 colores + ANSI 16  

### Debilidades Críticas (Auditoría)
⚠️ **Raw pointers en globals** (`g_terminal`, `g_input`) - Riesgo de memory leaks  
⚠️ **Headers duplicados** entre `/src/` y `/include/` - Mantenibilidad  
⚠️ **Funciones muy largas** (>50 líneas) en algunos componentes  
⚠️ **Testing insuficiente** (33% cobertura vs 80% estándar industrial)  
⚠️ **Documentación limitada** para nuevos contribuidores  

---

## 🎯 Líneas de Trabajo Prioritarias

### 🔥 PRIORIDAD P0: Estabilización Crítica (v0.3.1 - 2 semanas)

**Filosofía:** "Primero arreglar la base, luego construir"

#### 1.1 Eliminación de Raw Pointers Globals
*Impacto: ALTO | Esfuerzo: BAJO | Riesgo: BAJO*

**Problema:** Variables globales `g_terminal` y `g_input` usan raw pointers, violando RAII.

**Soluciones Propuestas:**
```cpp
// Opción A: Smart pointers singleton (Recomendada)
std::unique_ptr<ITerminal> g_terminal;
std::unique_ptr<IInput> g_input;

// Opción B: Referencias estáticas con lazy initialization
ITerminal& getTerminal() {
    static TerminalImpl instance;
    return instance;
}

// Opción C: Context object inyectado
class DesktopContext {
    std::unique_ptr<ITerminal> terminal;
    std::unique_ptr<IInput> input;
};
```

**Criterio de Decisión:** Opción A para compatibilidad backward, migrar a C gradualmente.

**Métrica de Éxito:** 0 raw pointers en globals, Valgrind clean.

---

#### 1.2 Consolidación de Headers Duplicados
*Impacto: MEDIO | Esfuerzo: BAJO | Riesgo: BAJO*

**Problema:** Headers duplicados entre directorios causan confusión y mantenimiento doble.

**Acciones:**
- [ ] Auditoría completa con script: `find . -name "*.hpp" | sort | xargs md5sum`
- [ ] Unificar fuente de verdad: `/include/desktop/` para API pública
- [ ] `/src/` solo para implementación privada
- [ ] Documentar estructura de directorios en CONTRIBUTING.md

**Métrica de Éxito:** 0 duplicados funcionales, estructura clara documentada.

---

#### 1.3 Optimización de Render Loop (Hot Path)
*Impacto: ALTO | Esfuerzo: MEDIO | Riesgo: BAJO*

**Problema:** Auditoría detectó asignaciones dinámicas innecesarias en `Renderer::flush()`.

**Técnicas Propuestas:**

**A. Stack Buffer para Escrituras Comunes**
```cpp
// Antes: std::string buffer (heap allocation)
// Después: std::array<char, 4096> buffer (stack allocation)
void Renderer::flush() {
    std::array<char, 4096> local_buffer;
    // Usar buffer para escrituras < 4KB (caso común)
    // Fallback a heap solo si excede capacidad
}
```

**B. Retorno por Referencia en Contenedores**
```cpp
// Antes: std::vector<WindowPtr> windows() const (copia profunda)
// Después: const std::vector<WindowPtr>& windows() const (referencia)
const auto& windows = desktop.windows(); // Zero copy
```

**C. Dirty Region Merging**
```cpp
// Unir rectángulos adyacentes antes de renderizar
std::vector<Rect> merged = mergeAdjacent(dirty_regions);
// Reduce syscalls write() al terminal
```

**Métrica de Éxito:** Reducir allocs/sec de ~100K a <10K en idle/render estático.

---

#### 1.4 Habilitar Sanitizers en CI
*Impacto: ALTO | Esfuerzo: BAJO | Riesgo: MEDIO*

**Configuración Propuesta:**
```yaml
# .github/workflows/ci.yml
jobs:
  build-sanitized:
    strategy:
      matrix:
        sanitizer: [address, undefined, thread]
    steps:
      - uses: actions/checkout@v4
      - name: Build with ${{ matrix.sanitizer }}
        run: |
          cmake -B build -DCMAKE_BUILD_TYPE=Debug \
                -DDESKTOP_ENABLE_SANITIZER=${{ matrix.sanitizer }}
          cmake --build build
          ctest --test-dir build --output-on-failure
```

**Build Preset Local:**
```bash
cmake --preset Debug-ASan   # AddressSanitizer
cmake --preset Debug-UBSan  # UndefinedBehaviorSanitizer
cmake --preset Debug-TSAN   # ThreadSanitizer
```

**Métrica de Éxito:** 0 warnings en todos los sanitizers, CI verde.

---

#### 1.5 Sistema de Logging Unificado
*Impacto: MEDIO | Esfuerzo: BAJO | Riesgo: BAJO*

**Diseño Propuesto:**
```cpp
// include/desktop/log.hpp
namespace dt {
    enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };
    
    class Logger {
    public:
        static Logger& instance();
        void setLevel(LogLevel level);
        void log(LogLevel level, std::string_view msg);
        
        // Macro-friendly API
        template<LogLevel L>
        struct Stream {
            Stream& operator<<(const auto& value) { /* ... */ return *this; }
            ~Stream() { Logger::instance().log<L>(buffer.str()); }
        };
    };
}

// Uso:
DT_LOG(INFO) << "Window created: " << window->id();
DT_LOG(DEBUG) << "Event dispatched: " << event.type;
```

**Features:**
- Niveles configurables en runtime
- Output a stderr o archivo
- Zero overhead en release (compilado fuera)
- Thread-safe opcional

**Métrica de Éxito:** Overhead <1% en release, logs útiles en debug.

---

### 🚀 PRIORIDAD P1: Features Core (v0.4.0 - 6-8 semanas)

#### 2.1 Sistema de Temas JSON
*Impacto: MEDIO (UX) | Esfuerzo: MEDIO | Riesgo: BAJO*

**Especificación:**
```json
{
  "name": "dracula",
  "version": "1.0",
  "colors": {
    "background": "#282a36",
    "foreground": "#f8f8f2",
    "primary": "#bd93f9",
    "secondary": "#ff79c6",
    "success": "#50fa7b",
    "warning": "#f1fa8c",
    "error": "#ff5555"
  },
  "styles": {
    "window.border": "double",
    "window.padding": 1,
    "button.align": "center"
  }
}
```

**API Propuesta:**
```cpp
ThemeManager::load("themes/dracula.json");
ThemeManager::reload(); // Hot reload
ThemeManager::set("default"); // Switch theme
```

**Métrica de Éxito:** Cambiar tema completo recargando JSON sin recompilar.

---

#### 2.2 Widgets Avanzados
*Impacto: MEDIO | Esfuerzo: MEDIO | Riesgo: BAJO*

**Widgets a Implementar:**

**A. Grid/Table Widget**
```cpp
auto table = std::make_shared<TableWidget>();
table->addColumn("Nombre", ColumnAlign::Left, 20);
table->addColumn("CPU", ColumnAlign::Right, 8);
table->addRow({"Process A", "45%"});
table->setSortable(true);
table->setVirtualScroll(true); // Para datasets grandes
```

**B. TreeView Widget**
```cpp
auto tree = std::make_shared<TreeView>();
auto root = tree->addNode("Root");
auto child = root->addChild("Child 1");
child->setExpandable(true);
child->setCheckable(true);
```

**C. ProgressBar Widget**
```cpp
auto progress = std::make_shared<ProgressBar>();
progress->setValue(75); // 0-100
progress->setStyle(ProgressBarStyle::Animated);
progress->setIndeterminate(true); // Modo desconocido
```

**D. Dropdown/ComboBox**
```cpp
auto dropdown = std::make_shared<ComboBox>();
dropdown->addItem("Opción 1");
dropdown->addItem("Opción 2");
dropdown->setSearchEnabled(true); // Type-ahead search
```

**Métrica de Éxito:** 4 widgets nuevos estables con tests y ejemplos.

---

#### 2.3 Integration Tests Visuales
*Impacto: ALTO | Esfuerzo: MEDIO | Riesgo: BAJO*

**Framework Propuesto:**
```cpp
TEST(VisualRegression, WindowRendering) {
    MockTerminal terminal(80, 24);
    auto desktop = std::make_shared<Desktop>(&terminal);
    
    auto window = std::make_shared<Window>("Test", Rect{1,1,40,10});
    desktop->addWindow(window);
    
    desktop->render();
    
    // Comparar con snapshot esperado
    EXPECT_SNAPSHOT(terminal.buffer(), "window_basic.txt");
}
```

**Features:**
- Snapshots versionados en `/tests/snapshots/`
- Diff automático en CI si cambia output
- Generación de snapshots con flag `--update-snapshots`

**Métrica de Éxito:** 0 regresiones visuales no detectadas en 100 commits.

---

#### 2.4 Cobertura de Tests >80%
*Impacto: ALTO | Esfuerzo: MEDIO | Riesgo: BAJO*

**Estrategia:**
1. Integrar `gcov`/`lcov` en CI
2. Reporte HTML accesible en artifacts
3. Fail build si cobertura < threshold
4. Identificar archivos críticos sin tests

**Configuración CMake:**
```cmake
option(DESKTOP_ENABLE_COVERAGE "Enable code coverage" OFF)
if(DESKTOP_ENABLE_COVERAGE)
    target_compile_options(desktop PRIVATE --coverage)
    target_link_options(desktop PRIVATE --coverage)
endif()
```

**Métrica de Éxito:** Cobertura total >80%, crítica >95%.

---

#### 2.5 Ejemplos Completos Documentados
*Impacto: ALTO (Adopción) | Esfuerzo: BAJO | Riesgo: BAJO*

**Ejemplos Propuestos:**

1. **Hello World** (5 mins)
   - Crear ventana simple con texto
   
2. **Calculadora TUI** (30 mins)
   - Widget Grid para botones
   - Manejo de clicks
   - Layout responsive

3. **Monitor de Sistema** (1 hora)
   - Table widget para procesos
   - ProgressBar para CPU/RAM
   - Auto-refresh cada segundo

4. **Editor de Texto Simple** (2 horas)
   - Textarea widget
   - Scroll vertical/horizontal
   - Guardar/abrir archivos

5. **Dashboard de Métricas** (3 horas)
   - Múltiples ventanas
   - Gráficos ASCII
   - Temas personalizables

**Métrica de Éxito:** Usuario nuevo puede compilar y ejecutar ejemplos en <10 mins.

---

### 🌟 PRIORIDAD P2: Ecosistema (v1.0.0 - 3-4 meses)

#### 3.1 SDK de Plugins Profesional
*Impacto: ALTO | Esfuerzo: ALTO | Riesgo: MEDIO*

**Arquitectura Propuesta:**
```cpp
// Plugin interface (ABI estable)
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual const char* name() const = 0;
    virtual const char* version() const = 0;
    virtual void init(IDesktopContext* ctx) = 0;
    virtual void shutdown() = 0;
};

// Macro de registro
#define DESKTOP_PLUGIN_ENTRY(PluginClass) \
    extern "C" IPlugin* create_plugin() { return new PluginClass(); }
```

**Sandboxing:**
- Limitar filesystem a `/tmp/desktop-plugins/<pid>/`
- Prevenir syscalls peligrosos (ptrace, etc.)
- Timeout en callbacks (watchdog timer)

**Hot-Reload:**
```cpp
PluginManager::watch("plugins/myplugin.so");
// Detecta cambios y recarga automáticamente
```

**Métrica de Éxito:** 3 plugins de terceros funcionales en primer mes.

---

#### 3.2 Soporte Android Completo
*Impacto: MEDIO (Nicho) | Esfuerzo: MEDIO | Riesgo: MEDIO*

**Features:**
- [ ] Gestos táctiles (swipe, pinch)
- [ ] Teclado virtual inteligente
- [ ] Build NDK oficial (`build-android.sh`)
- [ ] Adaptación portrait/landscape
- [ ] Testing en dispositivo físico

**Métrica de Éxito:** APK funcional probado en Termux.

---

#### 3.3 Documentación Profesional
*Impacto: ALTO | Esfuerzo: BAJO | Riesgo: BAJO*

**Entregables:**
- [ ] Doxygen auto-generado (HTML + PDF)
- [ ] Tutorial "Getting Started" paso a paso
- [ ] Diagramas de arquitectura (Mermaid)
- [ ] FAQ y troubleshooting
- [ ] Video tutorials (opcional)

**Métrica de Éxito:** Usuario nuevo productivo en <10 mins sin leer código fuente.

---

## 💡 Ideas "Wild Card" (Innovación Disruptiva)

### 4.1 Modo Presentación/Grabación
*Impacto: MEDIO | Esfuerzo: MEDIO | Riesgo: BAJO*

**Concepto:** Grabar sesión TUI a video ASCII o GIF animado.

**Casos de Uso:**
- Tutoriales y demos
- Debugging remoto
- Presentaciones en vivo

**Implementación:**
```cpp
Recorder::start("session.cast");
// ... interacción normal ...
Recorder::stop();
// Exporta a: .cast (formato propio), .gif, .mp4
```

---

### 4.2 Red Distribuida (VNC-like en Texto)
*Impacto: ALTO | Esfuerzo: MUY ALTO | Riesgo: ALTO*

**Concepto:** Compartir ventanas entre instancias remotas.

**Protocolo:**
- TCP/UDP personalizado
- Compresión de diffs (solo cambios)
- Autenticación opcional

**Casos de Uso:**
- Colaboración en tiempo real
- Remote debugging
- pair programming en TUI

---

### 4.3 AI Assistant Plugin
*Impacto: MEDIO | Esfuerzo: ALTO | Riesgo: MEDIO*

**Concepto:** LLM local integrado que sugiere comandos/layouts.

**Features:**
- Análisis de contexto de UI
- Sugerencias de atajos
- Generación automática de layouts

**Requisitos:**
- LLM local (Ollama, llama.cpp)
- Plugin architecture robusta
- Privacy-first (todo local)

---

### 4.4 Scripting Lua/Python Incrustado
*Impacto: MEDIO | Esfuerzo: ALTO | Riesgo: MEDIO*

**Concepto:** Automatización de UI sin compilar C++.

**API Ejemplo (Lua):**
```lua
desktop.create_window("Mi Ventana", 1, 1, 40, 10)
desktop.add_label(1, 1, "Hola Mundo")
desktop.on_click(function() 
    print("Click!") 
end)
```

**Ventajas:**
- Prototipado rápido
- Macros personalizables
- Plugins scripteables

**Desventajas:**
- Añade dependency (lua/python)
- Complejidad de build
- Performance overhead

---

## 📊 Matriz de Decisión: Impacto vs Esfuerzo

| Idea | Impacto | Esfuerzo | ROI | Prioridad | Sprint |
|------|---------|----------|-----|-----------|--------|
| **Smart Pointers Globals** | 🔥 10 | 🟢 2 días | ⭐⭐⭐⭐⭐ | **P0** | v0.3.1 |
| **Headers Duplicados** | 🟡 7 | 🟢 2 días | ⭐⭐⭐⭐ | **P0** | v0.3.1 |
| **Stack Buffer Renderer** | 🔥 9 | 🟡 4 días | ⭐⭐⭐⭐⭐ | **P0** | v0.3.1 |
| **Sanitizers CI** | 🔥 10 | 🟢 3 días | ⭐⭐⭐⭐⭐ | **P0** | v0.3.1 |
| **Logging System** | 🟡 7 | 🟢 3 días | ⭐⭐⭐⭐ | **P0** | v0.3.1 |
| **Integration Tests** | 🔥 9 | 🟡 6 días | ⭐⭐⭐⭐⭐ | **P1** | v0.4.0 |
| **Temas JSON** | 🟡 7 | 🟡 5 días | ⭐⭐⭐⭐ | **P1** | v0.4.0 |
| **Widgets Avanzados** | 🟡 8 | 🟡 8 días | ⭐⭐⭐⭐ | **P1** | v0.4.0 |
| **Cobertura >80%** | 🔥 8 | 🟡 6 días | ⭐⭐⭐⭐ | **P1** | v0.4.0 |
| **Ejemplos Completos** | 🟡 8 | 🟡 4 días | ⭐⭐⭐⭐⭐ | **P1** | v0.4.0 |
| **SDK Plugins** | 🔥 9 | 🔴 15 días | ⭐⭐⭐⭐ | **P2** | v1.0.0 |
| **Hot-Reload Plugins** | 🟡 6 | 🔴 12 días | ⭐⭐⭐ | **P2** | v1.0.0 |
| **Android Gestos** | 🟡 6 | 🟡 7 días | ⭐⭐⭐ | **P2** | v1.0.0 |
| **Doxygen Docs** | 🟡 7 | 🟢 2 días | ⭐⭐⭐⭐ | **P1** | v0.4.0 |
| **Scripting Lua** | 🟢 5 | 🔴 20 días | ⭐⭐ | **P3** | Futuro |
| **Red Distribuida** | 🟢 4 | 🔴 30 días | ⭐⭐ | **P3** | Futuro |

**Leyenda:**
- Impacto: 🔥 Alto (9-10) | 🟡 Medio (6-8) | 🟢 Bajo (1-5)
- Esfuerzo: 🟢 Bajo (1-4 días) | 🟡 Medio (5-8 días) | 🔴 Alto (9+ días)
- ROI: ⭐⭐⭐⭐⭐ Excelente | ⭐⭐⭐⭐ Muy Bueno | ⭐⭐⭐ Bueno

---

## 🗓️ Roadmap Consolidado

### Fase 1: Estabilización (v0.3.1) - 2 semanas
**Focus:** Deuda técnica, rendimiento, seguridad

**Entregables:**
- [ ] Smart pointers en globals
- [ ] Headers consolidados
- [ ] Stack buffer en renderer
- [ ] Sanitizers en CI
- [ ] Sistema de logging básico
- [ ] Documentación API core

**Métricas:**
- Allocs/sec: 100K → <10K
- Warnings ASan/UBSan: 0
- Headers duplicados: ~12 → 0
- Raw pointers globals: 2 → 0

---

### Fase 2: Features Core (v0.4.0) - 6-8 semanas
**Focus:** Funcionalidad visible, testing robusto

**Entregables:**
- [ ] Sistema de temas JSON
- [ ] 4 widgets avanzados (Table, Tree, Progress, Combo)
- [ ] Integration tests visuales
- [ ] Cobertura tests >80%
- [ ] 5 ejemplos completos
- [ ] Doxygen documentation

**Métricas:**
- Temas cambiables sin recompilar
- Cobertura: 33% → >80%
- Widgets nuevos: 0 → 4
- Ejemplos: 1 → 5

---

### Fase 3: Madurez (v1.0.0) - 3-4 meses
**Focus:** Extensibilidad, comunidad, producción-ready

**Entregables:**
- [ ] SDK de plugins estable
- [ ] Sandboxing de plugins
- [ ] Hot-reload de plugins
- [ ] Soporte Android completo
- [ ] Marketplace plugins (beta)
- [ ] Documentación completa

**Métricas:**
- Plugins comunidad: 0 → 5+
- APK Termux funcional
- 0 crashes en 24h stress test
- Usuario nuevo productivo en <10 mins

---

### Fase 4: Innovación (v1.1.0+) - TBD
**Focus:** Características disruptivas

**Candidatos:**
- [ ] Modo grabación/presentación
- [ ] AI assistant plugin
- [ ] Scripting Lua/Python
- [ ] Red distribuida experimental
- [ ] Backend Wayland nativo

---

## 🛠️ Plan de Acción Inmediato (Próximos 7 Días)

### Día 1-2: Setup y Baseline
- [ ] Crear branch `release/v0.3.1`
- [ ] Ejecutar auditoría headers duplicados
- [ ] Configurar build con sanitizers local
- [ ] Baseline benchmarks (allocs/sec, syscalls)
- [ ] Revisar issues abiertos en GitHub

### Día 3-4: Optimizaciones Críticas
- [ ] Implementar smart pointers en globals
- [ ] Stack buffer en `Renderer::flush()`
- [ ] Retornos por referencia en contenedores
- [ ] Medir mejora con benchmarks

### Día 5-6: Seguridad y CI
- [ ] Fixear warnings ASan/UBSan
- [ ] Configurar GitHub Actions matrix
- [ ] Integrar sanitizers en pipeline CI
- [ ] Validar en Linux, macOS, Windows

### Día 7: Documentación y Release Prep
- [ ] Documentar API pública core
- [ ] Actualizar CHANGELOG.md
- [ ] Preparar release notes v0.3.1
- [ ] Code review final
- [ ] Merge a main y tag v0.3.1

---

## 📈 Métricas de Seguimiento Continuo

| Métrica | v0.3.0 | v0.3.1 | v0.4.0 | v1.0.0 |
|---------|--------|--------|--------|--------|
| **Allocs/sec (idle)** | ~100K | <10K | <5K | <1K |
| **Cobertura Tests** | 33% | 35% | >80% | >90% |
| **Warnings Sanitizers** | ? | 0 | 0 | 0 |
| **Headers Duplicados** | ~12 | 0 | 0 | 0 |
| **Raw Pointers Globals** | 2 | 0 | 0 | 0 |
| **Tiempo Compilación** | X | X-15% | X-20% | X-25% |
| **Widgets Disponibles** | 5 básicos | 5 básicos | 9 (+4) | 12+ |
| **Ejemplos Completos** | 1 demo | 1 demo | 5 ejemplos | 10+ |
| **Plugins Comunidad** | 0 | 0 | 0 | 5+ |
| **Documentación Pages** | Básica | README+ | Doxygen+ | Full site |

---

## ⚠️ Análisis de Riesgos

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|--------------|---------|------------|
| Breaking changes en API | Media | Alto | Versionado semántico estricto, guía migración |
| Optimizaciones introducen bugs | Media | Medio | Tests exhaustivos post-cambio, rollback plan |
| CI se vuelve lento (>30 min) | Baja | Medio | Parallel jobs, cache estratégico |
| Sanitizers encuentran issues críticos | Alta | Alto | Time buffer (2 días extra) para fixes |
| Falta de adopción de plugins | Media | Medio | Crear plugins ejemplo atractivos |
| Complejidad en Android testing | Media | Bajo | Emuladores + beta testers dispositivos reales |

---

## 🎯 Criterios de Éxito por Fase

### v0.3.1 (Estabilización) ✅
- Todos tests pasan con sanitizers limpios
- Benchmarks muestran ≥90% reducción en allocs
- 0 headers duplicados
- CI verde en todas las plataformas
- 0 raw pointers en globals

### v0.4.0 (Features Core) ✅
- Sistema de temas funcional y documentado
- 4 widgets nuevos estables con tests
- Cobertura de tests >80%
- 5 ejemplos completos funcionando
- Doxygen auto-generado publicado

### v1.0.0 (Madurez) ✅
- SDK de plugins estable con 3 ejemplos
- Sandboxing funcional
- Documentación profesional completa
- 0 crashes en 24h de stress testing
- Comunidad activa (issues, PRs, plugins)

---

## 📞 Comunicación y Seguimiento

### Rituals Sugeridos
- **Daily Standup:** 15 mins (async preferred)
- **Weekly Review:** Demo progreso semanal (viernes)
- **Sprint Planning:** Inicio de cada fase
- **Retrospective:** Final de cada release

### Herramientas Recomendadas
- **Issue Tracking:** GitHub Projects con labels por prioridad
- **CI/CD:** GitHub Actions (matrix builds)
- **Documentation:** MkDocs + Doxygen
- **Communication:** Discord/Slack para comunidad
- **Benchmarks:** Google Benchmark integrado

---

## 🚀 Conclusión y Llamado a la Acción

**Recomendación Principal:** Comenzar **INMEDIATAMENTE** con **Fase 1 (v0.3.1)**.

**Razones:**
1. Las optimizaciones de rendimiento son **prerrequisitos** para features adicionales
2. La deuda técnica (raw pointers, headers duplicados) dificulta desarrollo futuro
3. Una base sólida y rápida atraerá más contribuidores
4. Los fixes de seguridad no pueden posponerse

**Primer Paso Concreto:**
```bash
cd /workspace
git checkout -b release/v0.3.1

# 1. Auditoría de headers duplicados
find include src -name "*.hpp" -o -name "*.h" | sort | xargs md5sum | sort -k2

# 2. Configurar build con sanitizers
cmake -B build-san -DCMAKE_BUILD_TYPE=Debug -DDESKTOP_ENABLE_ASAN=ON
cmake --build build-san

# 3. Ejecutar tests con sanitizers
ctest --test-dir build-san --output-on-failure

# 4. Baseline de benchmarks
./build-san/tests/desktop_benchmarks --benchmark_out=baseline.json
```

**¿Qué línea de trabajo priorizamos para comenzar hoy?**

---

## 📚 Recursos y Referencias

### Documentos del Proyecto
- `/workspace/CODE_QUALITY_AUDIT.md` - Auditoría detallada
- `/workspace/brainstorming_next_steps.md` - Brainstorm original
- `/workspace/BRAINSTORM_CONSOLIDADO_v0.3.0.md` - Plan consolidado anterior
- `/workspace/docs/` - Documentación técnica

### Herramientas de Profiling
- **Linux:** `valgrind --leak-check=full`, `perf record`, `heaptrack`, `strace -c`
- **macOS:** Instruments (Time Profiler, Allocations, Leaks)
- **Windows:** Visual Studio Profiler, ETW, Process Monitor

### Lecturas Recomendadas
- "Effective Modern C++" - Scott Meyers (C++17 best practices)
- "Game Engine Architecture" - Jason Gregory (object pooling, ECS)
- "The UNIX Programming Environment" - Kernighan & Pike (Unix philosophy)
- "Building Secure and Reliable Systems" - Google (security best practices)

---

**Documento Vivo:** Este documento se actualizará conforme avanza el desarrollo y surgen nuevas ideas.

**Última Actualización:** Abril 2026  
**Contribuidores:** Equipo Desktop TUI  
**Licencia:** MIT License

---

## 📝 Apéndice: Checklist de Tareas Inmediatas

### Semana 1 (v0.3.1 Sprint 1)
- [ ] Crear branch release/v0.3.1
- [ ] Identificar headers duplicados con script
- [ ] Configurar build preset Debug-ASan
- [ ] Baseline de benchmarks (allocs, syscalls, FPS)
- [ ] Refactorizar g_terminal → std::unique_ptr
- [ ] Refactorizar g_input → std::unique_ptr
- [ ] Implementar stack buffer en Renderer::flush()
- [ ] Cambiar retornos por valor a referencia constante
- [ ] Ejecutar tests con ASan y fixear warnings
- [ ] Ejecutar tests con UBSan y fixear warnings
- [ ] Configurar GitHub Actions matrix (Ubuntu, macOS, Windows)
- [ ] Integrar sanitizers en pipeline CI
- [ ] Diseñar API básica de logging
- [ ] Implementar Logger::instance()
- [ ] Integrar logs en creación/destrucción de ventanas
- [ ] Documentar API pública core (README ampliado)
- [ ] Actualizar CHANGELOG.md con cambios v0.3.1
- [ ] Preparar release notes
- [ ] Code review final
- [ ] Merge a main y tag v0.3.1

### Métricas de Éxito Semana 1
- [ ] Allocs/sec: 100K → <10K (verificar con benchmark)
- [ ] Warnings ASan: 0
- [ ] Warnings UBSan: 0
- [ ] Headers duplicados: ~12 → 0
- [ ] Raw pointers globals: 2 → 0
- [ ] CI verde en las 3 plataformas principales
- [ ] Tiempo compilación: -15% vs baseline

---

**¡Manos a la obra! 🚀**
