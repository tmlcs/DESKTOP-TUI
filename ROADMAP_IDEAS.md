# 🧠 Tormenta de Ideas: Hoja de Ruta Desktop TUI
## Planificación Estratégica con Enfoque SemVer

**Versión Actual:** `v0.3.0` (Alpha/Beta temprana)  
**Objetivo:** Definir el camino hacia `v1.0.0` (Stable) y más allá.

---

## 📊 Estado Actual y Contexto SemVer

Según [SemVer 2.0.0](https://semver.org/):
- **MAJOR (X.0.0):** Cambios incompatibles hacia atrás.
- **MINOR (x.Y.0):** Nuevas funcionalidades compatibles hacia atrás.
- **PATCH (x.y.Z):** Correcciones de bugs compatibles hacia atrás.

Dado que estamos en `0.x.y`, la API pública **no es estable**. Podemos hacer cambios "breaking" en versiones menores hasta llegar a `1.0.0`.

---

## 🗺️ Fases Propuestas

### Fase 1: Consolidación del Core (Hacia v0.4.0 - v0.5.0)
*Objetivo: Eliminar deuda técnica, completar features básicos y estabilizar API interna.*

#### 🟢 Patch Candidates (v0.3.x)
- [ ] **Fix: Optimización de memoria en `Renderer`**: Evitar reallocaciones frecuentes del buffer.
- [ ] **Fix: Manejo de señales de redimensionamiento**: Prevenir race conditions en `SIGWINCH`.
- [ ] **Docs: Ejemplos de uso para `EventBus` y `Signal/Slot`**.

#### 🔵 Minor Candidates (v0.4.0)
- [ ] **Feature: Widget `Button`**: Implementación básica con estados (hover, focus, pressed).
- [ ] **Feature: Widget `Checkbox` y `RadioButton`**: Para formularios simples.
- [ ] **Refactor: Unificar sistema de estilos**: Crear `StyleSheet` simple (similar a CSS inline).
- [ ] **Feature: Soporte para colores RGB reales (True Color)**: Detectar capacidad de terminal y habilitar 24-bit color.
- [ ] **Infra: Mejorar cobertura de tests**: Llegar al 90% en módulos core (`renderer`, `input`, `window`).

#### 🟠 Breaking Changes Preparados (para v0.5.0 o v1.0.0)
- [ ] **API Change: Renombrar `tui::core::Rect` métodos**: Estandarizar a `x(), y(), w(), h()` en lugar de `left, top, right, bottom` si es confuso.
- [ ] **API Change: Unificar eventos de Mouse**: Simplificar la estructura `MouseEvent` actual.

---

### Fase 2: Experiencia de Usuario y Widgets (Hacia v0.6.0 - v0.8.0)
*Objetivo: Proveer un set de widgets completo para construir aplicaciones reales.*

#### 🔵 Minor Candidates (v0.6.0)
- [ ] **Feature: Widget `Menu Bar` y `Dropdown`**: Menús desplegables clásicos de escritorio.
- [ ] **Feature: Widget `ProgressBar`**: Con soporte para modo indeterminado.
- [ ] **Feature: Layouts Automáticos**: `VBoxLayout`, `HBoxLayout` (similar a Qt o Flutter) para evitar posicionamiento manual absoluto.
- [ ] **Feature: Sistema de Temas**: Light/Dark mode y posibilidad de cargar temas personalizados (JSON/YAML).

#### 🔵 Minor Candidates (v0.7.0)
- [ ] **Feature: Widget `Table` / `Grid`**: Para mostrar datos tabulares con columnas ordenables.
- [ ] **Feature: Scrollbars personalizables**: Estilos y comportamiento configurable.
- [ ] **Feature: Tooltips**: Ayudas emergentes al hacer hover sobre widgets.

#### 🔵 Minor Candidates (v0.8.0)
- [ ] **Feature: Sistema de Clipboard mejorado**: Soporte para múltiples mime-types (texto, quizás imágenes en futuro lejano vía escapes oscuros).
- [ ] **Feature: Animaciones básicas**: Transiciones suaves para apertura/cierre de ventanas y cambios de foco.

---

### Fase 3: Madurez y Estabilización (Hacia v0.9.0 - v1.0.0)
*Objetivo: Congelar API, auditoría final y lanzamiento Stable.*

#### 🟠 Breaking Changes (v0.9.0 - "Release Candidate")
- [ ] **API Freeze**: Congelar firmas de funciones públicas.
- [ ] **Cleanup: Eliminar deprecated APIs**: Marcar APIs viejas como `[[deprecated]]` en v0.8.0 y eliminarlas en v0.9.0.
- [ ] **Refactor: Optimizaciones finales de rendimiento**: Profiling profundo para cuellos de botella.

#### 🏁 Major Release (v1.0.0)
- [ ] **Estabilidad de API Garantizada**: Compromiso de no romper compatibilidad hasta v2.0.0.
- [ ] **Documentación Completa**: Doxygen/Sphinx generado y guía de usuario detallada.
- [ ] **Auditoría de Seguridad Final**: Revisión exhaustiva de manejo de input y buffers.

---

### Fase 4: Futuro Post-1.0 (v1.x.x y v2.0.0)
*Objetivo: Expansión de capacidades y ecosistema.*

#### 🔵 Future Minors (v1.1.0+)
- [ ] **Feature: Widgets Gráficos**: Canvas básico para dibujar líneas, círculos, gráficos de barras.
- [ ] **Feature: Internacionalización (i18n)**: Soporte nativo para traducción de strings en widgets.
- [ ] **Feature: Bindings para otros lenguajes**: Python, Lua o Rust (vía C-API o FFIs).
- [ ] **Feature: Modo "Headless"**: Renderizar a buffer de texto/string para testing o generación de reportes sin terminal activa.

#### 🟠 Future Major (v2.0.0 - Hipotético)
- [ ] **Arquitectura Multi-threaded real**: Mover renderizado o lógica pesada a hilos secundarios (actualmente single-threaded UI).
- [ ] **Soporte para Protocolos Avanzados**: Kitty graphics protocol, Sixel, iTerm2 images (requiere breaking change en renderer).
- [ ] **Motor de Scripting integrado**: Lua o JS embebido para extender apps sin recompilar.

---

## 🚦 Matriz de Priorización (MoSCoW)

| Prioridad | Categoría | Elementos Clave | Impacto en SemVer |
|-----------|-----------|-----------------|-------------------|
| **MUST** | Crítico para v1.0 | Fix bugs críticos, Docs completas, API Stable, Tests >90% | Patch/Minor hasta freeze |
| **SHOULD** | Importante para UX | Layouts automáticos, Temas, Widgets básicos (Button, Menu) | Minor (0.x.x) |
| **COULD** | Deseable | Animaciones, Tooltips, i18n | Minor (0.x.x o 1.x.x) |
| **WON'T** | Fuera de alcance actual | Gráficos avanzados (Sixel), Bindings Python, Scripting | Major (2.0.0) o Plugins |

---

## 📋 Checklist de Lanzamiento v1.0.0

Para considerar el proyecto "Stable", debemos cumplir:

- [ ] **API Pública Documentada**: Todos los headers en `include/` con comentarios Doxygen claros.
- [ ] **Cobertura de Tests**: >90% en core, >80% en UI.
- [ ] **Ejemplos Funcionales**: Al menos 5 ejemplos cubriendo casos de uso comunes (form, dashboard, editor simple).
- [ ] **Guía de Migración**: Documento explicando cambios desde 0.x a 1.0 si hubo breaking changes.
- [ ] **Política de Versionado Clarificada**: Documento `VERSIONING.md` explicando cómo se manejan las versiones futuras.
- [ ] **Sin Known Bugs Críticos**: Lista de issues cerrada o etiquetada claramente como "won't fix" o "postponed".

---

## 💡 Ideas Innovadoras (Wildcards)

1.  **"TUI Composer"**: Una herramienta CLI integrada para diseñar layouts visualmente y exportar código C++.
2.  **Modo "Compatibilidad Legacy"**: Un flag para emular terminales antiguos (VT100 estricto) vs modernos (xterm-kitty).
3.  **Plugin System**: Cargar widgets dinámicos desde `.so`/`.dll` en runtime.
4.  **Remote TUI**: Protocolo para renderizar la TUI en un cliente remoto vía websocket (útil para web terminals).

---

## 📅 Cronograma Sugerido (Estimado)

| Versión | Fecha Estimada | Foco Principal |
|---------|----------------|----------------|
| v0.4.0 | +1 mes | Widgets básicos (Button, Checkbox) + True Color |
| v0.5.0 | +2 meses | Layouts Automáticos + Refactor API |
| v0.6.0 | +3 meses | Menús + Temas |
| v0.8.0 | +5 meses | Feature complete (Tablas, Animaciones) |
| v0.9.0 | +6 meses | RC1, API Freeze, Deprecación masiva |
| **v1.0.0** | **+7 meses** | **Lanzamiento Stable** |

> *Nota: Las fechas son orientativas y dependen de la disponibilidad de contribuyentes.*

---

## 🎯 Conclusión y Siguiente Paso Inmediato

**Recomendación:** Comenzar inmediatamente con la **Fase 1**, priorizando la implementación de **Layouts Automáticos** y **Widgets Básicos faltantes**, ya que son fundamentales para que los usuarios externos puedan empezar a adoptar la librería seriamente antes del v1.0.0.

**Acción inmediata sugerida:**
1. Crear issue en GitHub: "Roadmap to v1.0.0".
2. Etiquetar issues existentes con etiquetas SemVer (`v0.4`, `v1.0`, `breaking-change`).
3. Empezar implementación de `VBoxLayout` y `Button`.
