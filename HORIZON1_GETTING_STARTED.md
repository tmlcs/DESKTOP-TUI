# 🚀 Guía de Inicio para el Horizonte 1 - Estabilización v0.3.1

## Resumen Ejecutivo

Este documento proporciona instrucciones paso a paso para comenzar con la implementación del **Horizonte 1: Estabilización v0.3.x** del proyecto Desktop TUI.

**Duración:** 2 semanas (10 días laborables)  
**Versión Objetivo:** v0.3.1  
**Estado Actual:** v0.3.0  

---

## 📋 Checklist de Inicio Rápido

### Paso 1: Configurar Entorno de Desarrollo

```bash
# Ejecutar script de configuración automática
./scripts/setup_dev_env.sh
```

Esto creará:
- ✅ `build-debug/` - Build estándar con tests
- ✅ `build-asan/` - Build con sanitizers (ASan + UBSan)
- ✅ `build-tidy/` - Build para análisis estático
- ✅ `compile_commands.json` - Para IDEs y herramientas

### Paso 2: Verificar la Configuración

```bash
# Probar build debug
cd build-debug && ninja && ctest --output-on-failure

# Probar build con sanitizers
cd ../build-asan && ninja && ctest --output-on-failure

# Verificar formato de código
cd ..
find src include -name "*.cpp" -o -name "*.hpp" | xargs clang-format --dry-run --Werror
```

### Paso 3: Revisar Issues Prioritarios

Los issues están documentados en: `roadmap/horizon1_github_issues.md`

**Prioridad Crítica (Semana 1):**
- [ARCH-01] Auditoría de Headers
- [ARCH-02] Estandarización de Includes
- [PERF-01] Stack Buffer en Renderer::flush()
- [SEC-05] Integración de Sanitizers en CI

**Prioridad Alta (Semana 2):**
- [PERF-02] Retorno por Referencia en Contenedores
- [PERF-03] Dirty Regions Optimization
- [DOC-01] Documentación API con Doxygen

---

## 🎯 Primeras Tareas Detalladas

### Tarea 1: [ARCH-01] Auditoría de Headers (Día 1)

**Objetivo:** Identificar y eliminar headers duplicados entre `/src/` e `/include/`.

**Comandos útiles:**
```bash
# Listar todos los headers
find include src -name "*.hpp" -o -name "*.h" | sort

# Buscar posibles duplicados por nombre
find include src -name "*.hpp" -exec basename {} \; | sort | uniq -d

# Verificar includes en el código
grep -r "#include" src/ include/ --include="*.cpp" --include="*.hpp" | head -50
```

**Criterio de Aceptación:**
- [ ] No hay headers duplicados funcionales
- [ ] Todos los includes apuntan a la ubicación correcta
- [ ] El proyecto compila sin errores

---

### Tarea 2: [PERF-01] Stack Buffer en Renderer::flush() (Día 3-4)

**Objetivo:** Reducir asignaciones de heap en el hot path de renderizado.

**Archivo a modificar:** `include/ui/renderer.hpp`

**Problema actual:**
```cpp
// Línea ~288 en flush()
std::string run;  // Asignación de heap en cada iteración
for (int i = col; i < run_end; i++) {
    run += utf8_encode(back_buffer_[row * cols_ + i].ch);
}
```

**Solución propuesta:**
```cpp
// Usar stack buffer con fallback a heap solo si es necesario
std::array<char, 256> stack_buffer;
size_t offset = 0;

// Escribir directamente al stack buffer
for (int i = col; i < run_end && offset < stack_buffer.size() - 4; i++) {
    auto encoded = utf8_encode(back_buffer_[row * cols_ + i].ch);
    std::copy(encoded.begin(), encoded.end(), stack_buffer.begin() + offset);
    offset += encoded.size();
}

// Enviar al terminal
term_.write(std::string_view(stack_buffer.data(), offset));
```

**Benchmark esperado:**
- Antes: ~100,000 allocs/sec en render loop
- Después: <10,000 allocs/sec

**Comandos para benchmark:**
```bash
cd build-debug
ninja
./tests/desktop-tui-tests --benchmark_filter=Renderer
```

---

### Tarea 3: [SEC-05] Integración de Sanitizers en CI (Día 2)

**Objetivo:** Configurar GitHub Actions para ejecutar tests con ASan y UBSan.

**Archivo a verificar:** `.github/workflows/ci.yml`

**Verificación:**
```bash
# El workflow ya incluye:
# - Job 'sanitizers' con ASan + UBSan habilitados
# - Ejecución de valgrind para detección de memory leaks
# - Upload automático de logs en caso de fallo
```

**Prueba local:**
```bash
cd build-asan
ninja
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 ctest --output-on-failure
```

---

## 🛠️ Herramientas de Desarrollo

### Análisis Estático

```bash
# Clang-Tidy (análisis profundo)
clang-tidy src/ui/renderer.cpp -p build-tidy

# Ejecutar en todo el proyecto
find src -name "*.cpp" | xargs -I {} clang-tidy {} -p build-tidy
```

### Formato de Código

```bash
# Verificar formato (sin modificar)
clang-format --dry-run --Werror src/**/*.cpp include/**/*.hpp

# Aplicar formato automáticamente
clang-format -i src/**/*.cpp include/**/*.hpp
```

### Profiling

```bash
# Usando perf (Linux)
perf record -g ./build-debug/desktop-tui
perf report

# Usando valgrind
valgrind --tool=callgrind ./build-debug/desktop-tui
kcachegrind callgrind.out.*
```

---

## 📊 Métricas de Éxito del Sprint

| Métrica | Línea Base | Objetivo | Cómo Medir |
|---------|------------|----------|------------|
| Allocs/sec (render) | ~100K | <10K | heaptrack / massif |
| Headers duplicados | ~10-15 | 0 | `find ... \| uniq -d` |
| Warnings ASan/UBSan | Desconocido | 0 | CI job 'sanitizers' |
| Builds en CI | 3 configs | 6+ configs | GitHub Actions |
| Cobertura documentación | Parcial | 100% API pública | Doxygen warnings |

---

## 🔍 Solución de Problemas Comunes

### Error: "undefined reference to..."

**Causa:** Headers movidos pero CMakeLists.txt no actualizado.

**Solución:**
```bash
# Limpiar build
rm -rf build-*

# Regenerar
./scripts/setup_dev_env.sh
```

### Error: Sanitizer detecta memory leak

**Pasos:**
1. Leer el output de ASan cuidadosamente
2. Identificar el stack trace
3. Verificar si es un false positive
4. Si es real, añadir fix y test de regresión

### Error: clang-format cambia mucho código

**Solución:**
```bash
# Aplicar formato gradualmente por archivo
git ls-files '*.cpp' '*.hpp' | head -10 | xargs clang-format -i

# Commitear cambios de formato por separado
git commit -m "style: aplicar clang-format a componentes core"
```

---

## 📞 Recursos Adicionales

- **Plan detallado:** `roadmap/horizon1_stabilization_plan.md`
- **Issues completos:** `roadmap/horizon1_github_issues.md`
- **Auditoría de calidad:** `audit/CODE_QUALITY_AUDIT.md`
- **CI/CD Pipeline:** `.github/workflows/ci.yml`

---

## ✅ Definición de "Terminado" (DoD)

Una tarea se considera completada cuando:

- [ ] Código implementado y funcional
- [ ] Tests unitarios pasan (incluyendo sanitizers)
- [ ] Formato de código verificado (`clang-format`)
- [ ] Análisis estático limpio (`clang-tidy`)
- [ ] Documentación actualizada (si aplica)
- [ ] Code review aprobado (si hay equipo)
- [ ] Merge a rama `dev/v0.3.1`

---

## 🎉 ¡Comencemos!

1. Ejecuta `./scripts/setup_dev_env.sh`
2. Elige tu primera tarea de la lista
3. Crea una rama: `git checkout -b feature/ARCH-01-header-audit`
4. ¡Manos a la obra!

**Contacto:** Para dudas, revisar la documentación o abrir un issue en GitHub.
