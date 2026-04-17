# ✅ Checklist de Implementación - Horizonte 1 (v0.3.1)

## 📅 Sprint Overview

- **Inicio:** [Fecha de inicio]
- **Fin:** [Fecha de fin + 2 semanas]
- **Versión:** v0.3.1
- **Rama:** `dev/v0.3.1`

---

## Día 1-2: Limpieza de Arquitectura y Headers

### [ARCH-01] Auditoría de Headers
- [ ] Ejecutar script de detección de duplicados
- [ ] Listar todos los headers en `/src/` e `/include/`
- [ ] Identificar duplicados funcionales
- [ ] Decidir fuente de verdad para cada duplicado
- [ ] Eliminar headers duplicados
- [ ] Actualizar includes en todo el código
- [ ] Verificar compilación exitosa
- [ ] Commit: `arch: eliminar headers duplicados (#issue)`

### [ARCH-02] Estandarización de Includes
- [ ] Crear directriz documentada de organización de headers
- [ ] Mover headers según directriz
- [ ] Actualizar CMakeLists.txt si es necesario
- [ ] Verificar que no hay errores de include
- [ ] Commit: `arch: estandarizar estructura de includes`

### [ARCH-03] Refactorización de Dependencias
- [ ] Auditar uso de `#pragma once`
- [ ] Identificar dependencias circulares
- [ ] Reemplazar con forward declarations donde sea posible
- [ ] Medir tiempo de compilación antes/después
- [ ] Commit: `arch: reducir dependencias circulares`

**✅ Criterio de Terminación Día 2:**
- [ ] 0 headers duplicados
- [ ] Compilación exitosa en las 3 configuraciones (debug, asan, tidy)
- [ ] Tiempo de compilación reducido ≥15%

---

## Día 3-5: Optimización Crítica de Rendimiento

### [PERF-01] Stack Buffer en Renderer::flush()
- [ ] Profilear código actual para establecer línea base
- [ ] Implementar stack buffer en lugar de std::string
- [ ] Añadir bounds checking
- [ ] Ejecutar benchmarks comparativos
- [ ] Verificar que no hay regresiones con sanitizers
- [ ] Documentar optimización en código
- [ ] Commit: `perf: usar stack buffer en Renderer::flush()`

### [PERF-02] Retorno por Referencia en Contenedores
- [ ] Identificar todos los métodos que retornan contenedores por valor
- [ ] Cambiar a retorno por referencia constante
- [ ] Actualizar documentación de lifetime
- [ ] Verificar tests de rendimiento
- [ ] Commit: `perf: retornar contenedores por referencia`

### [PERF-03] Dirty Regions Optimization
- [ ] Implementar algoritmo de unión de rects adyacentes
- [ ] Integrar en Renderer::mark_dirty()
- [ ] Benchmark: número de syscalls write() antes/después
- [ ] Commit: `perf: unir dirty regions adyacentes`

### [PERF-04] Event Dispatching sin Copias
- [ ] Auditar cadena de event handling
- [ ] Pasar Event por const& en toda la cadena
- [ ] Usar std::move solo cuando sea necesario
- [ ] Profilear para verificar 0 copias innecesarias
- [ ] Commit: `perf: evitar copias en event dispatching`

**✅ Criterio de Terminación Día 5:**
- [ ] Allocs/sec < 10,000 (desde ~100,000)
- [ ] Benchmarks documentados en docs/performance/
- [ ] 0 warnings en sanitizers

---

## Día 6-7: Robustez y Sanitizers

### [SEC-05] Integración Completa de Sanitizers
- [ ] Verificar CI job 'sanitizers' funcionando
- [ ] Ejecutar tests locales con ASan+UBSan
- [ ] Fixear cualquier issue detectado
- [ ] Ejecutar valgrind para memory leaks
- [ ] Documentar issues encontrados y fixes
- [ ] Commit: `sec: integrar sanitizers en CI/CD`

### [SEC-06] Validación UTF-8 Estricta
- [ ] Auditar todas las funciones de string_utils.hpp
- [ ] Añadir validación de entrada en utf8_decode()
- [ ] Manejar casos de UTF-8 inválido gracefulmente
- [ ] Añadir tests de casos borde
- [ ] Commit: `sec: validación estricta de UTF-8`

### [SEC-07] Bounds Checking en Buffers
- [ ] Revisar todos los accesos a buffers
- [ ] Añadir assertions en modo debug
- [ ] Verificar que no hay out-of-bounds con ASan
- [ ] Commit: `sec: añadir bounds checking`

**✅ Criterio de Terminación Día 7:**
- [ ] 0 warnings ASan/UBSan en CI
- [ ] 0 memory leaks en valgrind
- [ ] Todos los tests passing

---

## Día 8: Logging Unificado

### [LOG-01] Sistema de Logs Unificado
- [ ] Diseñar interfaz de logging unificada
- [ ] Implementar niveles: TRACE, DEBUG, INFO, WARN, ERROR
- [ ] Añadir soporte para output a archivo/consola
- [ ] Migrar logger.hpp existente al nuevo sistema
- [ ] Documentar uso del logger
- [ ] Commit: `log: implementar sistema unificado de logs`

**✅ Criterio de Terminación Día 8:**
- [ ] Logger funcional en todos los componentes
- [ ] Documentación completa
- [ ] Tests de logging

---

## Día 9: Documentación API

### [DOC-01] Documentación Doxygen
- [ ] Verificar Doxyfile configurado
- [ ] Añadir comentarios Doxygen a componentes core:
  - [ ] event.hpp
  - [ ] signal.hpp
  - [ ] rect.hpp
  - [ ] colors.hpp
  - [ ] renderer.hpp
  - [ ] widget.hpp
  - [ ] desktop.hpp
  - [ ] window.hpp
- [ ] Generar documentación HTML
- [ ] Verificar 0 warnings de Doxygen
- [ ] Commit: `docs: añadir documentación API completa`

**✅ Criterio de Terminación Día 9:**
- [ ] 100% de API pública documentada
- [ ] 0 warnings en generación de docs
- [ ] Docs disponibles en docs/html/

---

## Día 10: Testing Final y Release Prep

### [TEST-01] Ejecución Completa de Tests
- [ ] Ejecutar todos los tests en build-debug
- [ ] Ejecutar todos los tests en build-asan
- [ ] Ejecutar todos los tests en build-tidy
- [ ] Verificar cobertura de tests
- [ ] Fixear cualquier fallo

### [RELEASE-01] Preparación para Release
- [ ] Actualizar CHANGELOG.md
- [ ] Actualizar VERSION a 0.3.1
- [ ] Crear release notes
- [ ] Taggear versión en git
- [ ] Merge a rama main

### [RETRO-01] Retrospectiva del Sprint
- [ ] Reunión de retrospectiva
- [ ] Documentar lecciones aprendidas
- [ ] Planificar Horizonte 2
- [ ] Celebrar éxitos 🎉

**✅ Criterio de Terminación Día 10:**
- [ ] 100% de tests passing
- [ ] v0.3.1 released
- [ ] Retrospectiva completada

---

## 📊 Métricas de Éxito del Sprint

| Métrica | Línea Base | Objetivo | Actual | Estado |
|---------|------------|----------|--------|--------|
| Allocs/sec (render) | ~100K | <10K | [ ] | ⬜ |
| Headers duplicados | ~10-15 | 0 | [ ] | ⬜ |
| Warnings ASan/UBSan | ? | 0 | [ ] | ⬜ |
| Builds en CI | 3 | 6+ | [ ] | ⬜ |
| API documentada | Parcial | 100% | [ ] | ⬜ |
| Tiempo de compilación | Base | -15% | [ ] | ⬜ |

---

## 🔗 Enlaces Útiles

- **Plan Detallado:** `horizon1_stabilization_plan.md`
- **Issues GitHub:** `horizon1_github_issues.md`
- **Guía de Inicio:** `../HORIZON1_GETTING_STARTED.md`
- **Auditoría Calidad:** `../audit/CODE_QUALITY_AUDIT.md`
- **CI/CD Config:** `../.github/workflows/ci.yml`

---

## 📝 Notas del Sprint

[Espacio para notas diarias, blockers, decisiones técnicas, etc.]

### Día 1:


### Día 2:


### Día 3:


### Día 4:


### Día 5:


### Día 6:


### Día 7:


### Día 8:


### Día 9:


### Día 10:

---

**Estado del Sprint:** ⬜ No Iniciado | 🟡 En Progreso | ✅ Completado

**Última Actualización:** [Fecha]
