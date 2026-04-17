# 🧠 Tormenta de Ideas: Próximos Pasos - Desktop TUI

**Fecha:** 2024
**Versión Actual:** v0.3.0
**Objetivo:** Definir la hoja de ruta para v0.4.0 (Minor) y v1.0.0 (Major)

---

## 📋 Contexto Estratégico

Basado en la auditoría de calidad (Score: B+, 82/100), el proyecto es **técnicamente sólido** pero requiere optimizaciones críticas y nuevas funcionalidades para alcanzar madurez de producción.

**Filosofía de Diseño:**
- ✅ Zero dependencies (mantener)
- ✅ C++17 puro (mantener hasta v2.0)
- ✅ Single-threaded core (arquitectura actual)
- ✅ Multi-plataforma nativa (POSIX, Windows, Android)

---

## 🚀 Líneas de Trabajo Propuestas

### 1. 🔥 Optimización de Rendimiento (Prioridad: CRÍTICA)
*Impacto: Alto | Esfuerzo: Medio | Riesgo: Bajo*

**Problema:** La auditoría detectó ~100K allocs/sec en el render loop y copias innecesarias de contenedores.

**Ideas:**
- [ ] **Stack Buffer para Render:** Reemplazar `std::string` dinámico con `std::array<char, 4096>` en `Renderer::flush()` para escrituras típicas.
- [ ] **Retorno por Referencia:** Cambiar `Desktop::windows()` y similares para retornar `const std::vector<WindowPtr>&` evitando copias profundas.
- [ ] **Object Pooling:** Implementar un pool estático para objetos `Event` y `Rect` frecuentemente creados/destruidos.
- [ ] **Dirty Regions Avanzadas:** Mejorar el algoritmo actual para unir rectángulos adyacentes antes de renderizar (reducir syscalls).
- [ ] **SIMD Rendering:** Usar intrínsecos AVX2/NEON para operaciones de llenado de buffers (memset acelerado).

**Metrica de Éxito:** Reducir asignaciones de heap en un 90% durante idle/render estático.

---

### 2. 🛡️ Robustez y Seguridad (Prioridad: ALTA)
*Impacto: Alto | Esfuerzo: Bajo | Riesgo: Medio*

**Problema:** Headers duplicados y falta de validación de límites en algunos paths edge-case.

**Ideas:**
- [ ] **Limpieza de Headers:** Eliminar duplicados en `/src/` vs `/include/`. Unificar en una sola fuente de verdad.
- [ ] **Fuzzing de Inputs:** Integrar `libFuzzer` o AFL para probar el parser de eventos de teclado/mouse con datos aleatorios.
- [ ] **Sanitizers en CI:** Habilitar ASan (AddressSanitizer) y UBSan (UndefinedBehaviorSanitizer) en el pipeline de GitHub Actions.
- [ ] **Timeout de Bloqueo:** Mecanismo de seguridad para evitar que un plugin o callback bloquee el mainloop indefinidamente (watchdog timer).
- [ ] **Validación UTF-8 Estricta:** Rechazar secuencias mal formadas en lugar de mostrar caracteres basura o corromper el layout.

**Metrica de Éxito:** 0 warnings en sanitizers, 0 crashes en 24h de fuzzing.

---

### 3. 🎨 Sistema de Temas y Estilos (Prioridad: MEDIA)
*Impacto: Medio (UX) | Esfuerzo: Medio | Riesgo: Bajo*

**Problema:** Los colores están "hardcoded" o son muy básicos. Falta personalización.

**Ideas:**
- [ ] **Motor de Temas JSON:** Cargar paletas de colores desde archivos `.json` externos (ej. "dracula", "nord", "gruvbox").
- [ ] **Estilos CSS-like Simplificado:** Sintaxis para definir bordes, padding y alineación de widgets (`border: double; color: @primary;`).
- [ ] **Soporte True Color (24-bit):** Asegurar degradados suaves y soporte para terminales modernos (iTerm2, Windows Terminal).
- [ ] **Iconos ASCII/Unicode:** Librería interna de iconos (carpetas, errores, warnings) renderizables en cualquier widget.
- [ ] **Modo Alto Contraste:** Detección automática o toggle manual para accesibilidad.

**Metrica de Éxito:** Capacidad de cambiar todo el look & feel recargando un archivo de configuración sin recompilar.

---

### 4. 🧩 Ecosistema de Plugins (Prioridad: MEDIA)
*Impacto: Alto (Extensibilidad) | Esfuerzo: Alto | Riesgo: Alto*

**Problema:** El sistema de plugins existe pero es básico. Falta documentación y ejemplos complejos.

**Ideas:**
- [ ] **SDK de Plugins:** Crear un repo separado `desktop-tui-plugin-sdk` con headers mínimos y ejemplos de build (CMake template).
- [ ] **Sandboxing Básico:** Limitar acceso a filesystem del plugin a un directorio específico (prevenir lectura de `/etc/shadow`).
- [ ] **Hot-Reload:** Detectar cambios en `.so/.dll` y recargar el plugin en tiempo de ejecución sin reiniciar la app principal.
- [ ] **Marketplace de Plugins:** Repositorio centralizado (GitHub org o simple lista JSON) donde usuarios compartan plugins.
- [ ] **Plugin "Terminal Emulator":** Un plugin avanzado que emule una terminal real dentro de una ventana (con shell subprocess).

**Metrica de Éxito:** 3 plugins de terceros funcionales desarrollados por la comunidad en el primer mes post-lanzamiento.

---

### 5. 📱 Experiencia Móvil (Android/Termux) (Prioridad: BAJA/MEDIA)
*Impacto: Medio (Nicho) | Esfuerzo: Medio | Riesgo: Medio*

**Problema:** El soporte Android existe pero no está probado exhaustivamente en dispositivos reales.

**Ideas:**
- [ ] **Gestos Táctiles:** Mapear swipes (deslizar) a acciones de escritorio (cambiar desktop, minimizar ventana).
- [ ] **Teclado en Pantalla:** Manejo inteligente del teclado virtual de Android (evitar que tape la UI).
- [ ] **Build System NDK:** Script oficial `build-android.sh` que compile binarios estáticos listos para Termux.
- [ ] **Adaptación de Layout:** Modo "Portrait" vs "Landscape" con reorganización automática de ventanas.

**Metrica de Éxito:** APK o binario funcional probado en Termux en dispositivo físico.

---

### 6. 🧪 Testing y Calidad (Prioridad: ALTA)
*Impacto: Alto (Mantenibilidad) | Esfuerzo: Medio | Riesgo: Bajo*

**Problema:** Ratio de testing 13% (bajo para estándares industriales). Faltan integration tests.

**Ideas:**
- [ ] **Integration Tests Visuales:** Framework que renderice escenas completas a buffer de texto y compare con "snapshots" esperados (regression testing).
- [ ] **Mock de Plataforma:** Clase `MockTerminal` que permita simular inputs de teclado/mouse programáticamente sin hardware real.
- [ ] **Cobertura de Código:** Integrar `gcov`/`lcov` en CI para reportar % de líneas cubiertas (Meta: >80%).
- [ ] **Tests de Estrés:** Script que abra/cierre 1000 ventanas rápidamente para detectar memory leaks o race conditions.

**Metrica de Éxito:** Cobertura de código >80%, 0 regresiones visuales en 100 commits.

---

### 7. 📚 Documentación y Developer Experience (Prioridad: ALTA)
*Impacto: Alto (Adopción) | Esfuerzo: Bajo | Riesgo: Bajo*

**Problema:** Falta documentación API y guías de "Getting Started".

**Ideas:**
- [ ] **Doxygen Auto-Generado:** Configurar Doxygen para generar HTML/PDF de toda la API pública.
- [ ] **Tutorial "Hello World":** Guía paso a paso para crear tu primera ventana en 5 minutos.
- [ ] **Ejemplos Completos:** Carpeta `/examples/` con apps reales (calculadora, editor de texto simple, dashboard de monitoreo).
- [ ] **Arquitectura Detallada:** Diagramas UML o Mermaid explicando el flujo de eventos y renderizado.
- [ ] **FAQ y Troubleshooting:** Sección común de problemas (ej. "¿Por qué no se ven los colores en PuTTY?").

**Metrica de Éxito:** Un usuario nuevo puede compilar y ejecutar un ejemplo en <10 mins sin leer el código fuente.

---

## 🗓️ Hoja de Ruta Sugerida (Roadmap)

### Fase 1: Limpieza y Estabilización (v0.3.1 - Patch)
*Duración estimada: 2 semanas*
- [ ] Eliminar headers duplicados.
- [ ] Fix de retornos por valor (optimización crítica).
- [ ] Habilitar Sanitizers en CI.
- [ ] Documentación básica de API (README ampliado).

### Fase 2: Rendimiento y Features Core (v0.4.0 - Minor)
*Duración estimada: 6-8 semanas*
- [ ] Implementar Stack Buffer en Renderer.
- [ ] Sistema de Temas JSON.
- [ ] Integration Tests visuales.
- [ ] Ejemplos completos (Calculadora, Monitor de Sistema).

### Fase 3: Madurez y Ecosistema (v1.0.0 - Major)
*Duración estimada: 3-4 meses*
- [ ] SDK de Plugins estable + Sandbox.
- [ ] Soporte completo Android (Gestos).
- [ ] Cobertura de tests >80%.
- [ ] Documentación Doxygen completa.
- [ ] Lanzamiento oficial "Stable".

---

## 💡 Ideas "Wild Card" (Innovación)

1.  **Modo "Presentación":** Grabar la sesión TUI a video ASCII o stream para tutoriales.
2.  **Red Distribuida:** Permitir que ventanas de diferentes instancias de Desktop TUI en distintas máquinas se vean entre sí (VNC-like pero en texto).
3.  **AI Assistant Plugin:** Integrar LLM local que sugiera comandos o layouts basados en el uso.
4.  **Scripting Lua/Python:** Incrustar intérprete para scripting de automatización de UI sin compilar C++.

---

## 📊 Matriz de Decisión (Impacto vs Esfuerzo)

| Idea | Impacto | Esfuerzo | Prioridad |
|------|---------|----------|-----------|
| Optimización Render (Stack Buffer) | 🔥 Alto | 🟢 Bajo | **P0** |
| Limpieza Headers / Sanitizers | 🔥 Alto | 🟢 Bajo | **P0** |
| Sistema de Temas | 🟡 Medio | 🟡 Medio | P1 |
| Integration Tests | 🔥 Alto | 🟡 Medio | **P1** |
| SDK Plugins | 🔥 Alto | 🔴 Alto | P2 |
| Soporte Android Gestos | 🟡 Medio | 🟡 Medio | P2 |
| Scripting Lua/Python | 🟢 Bajo | 🔴 Alto | P3 |

---

## 🎯 Conclusión y Siguiente Paso Inmediato

**Recomendación:** Comenzar inmediatamente con la **Fase 1 (v0.3.1)**.
Los fixes de rendimiento y limpieza de código son prerrequisitos para cualquier feature nueva. Sin una base sólida y rápida, las features adicionales solo añadirán deuda técnica.

**Acción Inmediata Propuesta:**
1.  Crear branch `release/v0.3.1`.
2.  Ejecutar fix de headers duplicados.
3.  Refactorizar `Renderer::flush()` para usar stack buffer.
4.  Validar con Valgrind/ASan.

¿Qué línea de trabajo te parece más prioritaria para comenzar?
