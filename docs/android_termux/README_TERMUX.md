# Guía Rápida: Desktop TUI en Termux/Android

## 🚀 Inicio Rápido (5 minutos)

### Prerrequisitos
- Dispositivo Android 7.0+
- Termux instalado (desde F-Droid recomendado)
- Conexión a internet

### Pasos de Instalación

```bash
# 1. Abrir Termux y actualizar
pkg update && pkg upgrade

# 2. Clonar el repositorio
git clone https://github.com/tu-usuario/desktop-tui.git
cd desktop-tui

# 3. Ejecutar script de configuración
chmod +x scripts/setup_termux.sh
./scripts/setup_termux.sh

# 4. Reiniciar Termux (importante!)
# Desliza desde el borde para cerrar completamente y vuelve a abrir

# 5. Aplicar cambios
source ~/.bashrc

# 6. Verificar instalación
dtui-check

# 7. Compilar
dtui-build

# 8. ¡Ejecutar!
dtui
```

## 📱 Comandos Esenciales

| Comando | Descripción |
|---------|-------------|
| `dtui` | Ejecutar Desktop TUI |
| `dtui-build` | Compilar proyecto |
| `dtui-clean` | Limpiar build anterior |
| `dtui-test` | Ejecutar suite de tests |
| `dtui-check` | Verificar capacidades del sistema |

## ⌨️ Configuración del Teclado

El script configura automáticamente un teclado extendido con:

**Fila superior:** `ESC TAB CTRL ALT - / ~`  
**Fila navegación:** `↑ ← ↓ → DELETE BACKSPACE`  
**Fila funciones:** `F1 F2 ... F12`

### Personalizar Teclas

Editar `~/.termux/termux.properties`:

```properties
extra-keys=[[ESC, TAB, CTRL, ALT, {key: '-', popup: '|'}],
            [UP, LEFT, DOWN, RIGHT, DELETE],
            [F1, F2, F3, F4, F5]]
```

## 🎨 Problemas Comunes

### Los colores no se ven bien

```bash
export TERM=xterm-256color
export COLORTERM=truecolor
```

### Las teclas F1-F12 no funcionan

Usa la fila de teclas extra configurada o combina:
- `ESC + 1` = F1
- `ESC + 2` = F2
- etc.

### El renderizado es lento

```bash
# Reducir resolución
stty cols 80 rows 24

# Deshabilitar animaciones
export DESKTOP_TUI_NO_ANIMATIONS=1
```

### Touch no responde bien

- Usa stylus para mayor precisión
- Evita gestos rápidos durante input de texto
- Ajusta sensibilidad en la configuración

## 🔍 Debugging

```bash
# Ver logs
logcat -s termux:*

# Modo debug
DESKTOP_TUI_DEBUG=1 dtui

# Modo seguro (sin plugins)
DESKTOP_TUI_SAFE_MODE=1 dtui

# Monitorear recursos
top -m 10
```

## 📊 Rendimiento Esperado

| Dispositivo | FPS Terminal | FPS X11 | RAM |
|-------------|--------------|---------|-----|
| Gama Alta | 30-40 | 60 | 20 MB |
| Gama Media | 20-30 | 55 | 25 MB |
| Gama Baja | 15-25 | 45 | 30 MB |

## 🛠️ Desarrollo

### Compilar desde cero

```bash
cd ~/desktop-tui
mkdir build && cd build
cmake .. -DPLATFORM=termux -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

### Ejecutar tests

```bash
cd build
ctest --output-on-failure
```

### Perfilado

```bash
# Usar simpleperf (incluido en Android)
simpleperf record ./desktop-tui
simpleperf report
```

## 📚 Recursos Adicionales

- [Guía completa de optimizaciones](OPTIMIZACIONES_TERMUX.md)
- [Documentación oficial de Termux](https://wiki.termux.com/)
- [Comunidad en GitHub](https://github.com/termux/termux-app/discussions)

## 🆘 Soporte

Si encuentras problemas:

1. Ejecuta `dtui-check` y guarda el output
2. Revisa logs: `logcat -s termux:* > logs.txt`
3. Abre un issue en GitHub con:
   - Modelo del dispositivo
   - Versión de Android
   - Versión de Termux
   - Output de `dtui-check`
   - Logs relevantes

---

**Versión:** v0.3.0  
**Última actualización:** 2024
