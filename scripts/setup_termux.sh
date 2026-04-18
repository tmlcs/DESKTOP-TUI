#!/data/data/com.termux/files/usr/bin/bash
# setup_termux.sh - Configuración automática para Desktop TUI
# Versión: 0.3.0
# Compatible con: Termux v0.118.0+, Android 7.0+

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}╔════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║   Desktop TUI - Setup para Termux         ║${NC}"
echo -e "${CYAN}╚════════════════════════════════════════════╝${NC}"
echo ""

# 1. Verificar instalación básica
echo -e "${BLUE}[1/7]${NC} Verificando entorno..."
if ! command -v pkg &> /dev/null; then
    echo -e "${RED}❌ Error: No estás en Termux o pkg no está disponible${NC}"
    echo -e "${YELLOW}💡 Si estás en Android, instala Termux desde F-Droid:${NC}"
    echo -e "   https://f-droid.org/en/packages/com.termux/"
    exit 1
fi

TERMUX_VERSION=$(pkg --version 2>&1 | head -n1 || echo "unknown")
echo -e "${GREEN}✓${NC} Termux detectado (versión: ${TERMUX_VERSION})"

# Verificar arquitectura
ARCH=$(uname -m)
echo -e "${GREEN}✓${NC} Arquitectura: ${ARCH}"

# 2. Instalar dependencias
echo -e "${BLUE}[2/7]${NC} Instalando dependencias..."

# Lista de paquetes requeridos
REQUIRED_PACKAGES=(
    "cmake"
    "ninja-build"
    "clang"
    "git"
    "python"
    "libandroid-support"
    "ncurses-utils"
    "termux-exec"
)

# Verificar cuáles ya están instalados
MISSING_PACKAGES=()
for pkg in "${REQUIRED_PACKAGES[@]}"; do
    if ! dpkg -l "$pkg" &> /dev/null; then
        MISSING_PACKAGES+=("$pkg")
    fi
done

if [ ${#MISSING_PACKAGES[@]} -eq 0 ]; then
    echo -e "${GREEN}✓${NC} Todas las dependencias ya están instaladas"
else
    echo -e "${YELLOW}📦 Paquetes a instalar:${NC} ${MISSING_PACKAGES[*]}"
    echo -e "${YELLOW}⚠️  Esto puede tardar varios minutos...${NC}"
    
    # Actualizar repositorios
    pkg update -y
    
    # Instalar paquetes faltantes
    pkg install -y "${MISSING_PACKAGES[@]}"
    
    echo -e "${GREEN}✓${NC} Dependencias instaladas correctamente"
fi

# 3. Configurar variables de entorno
echo -e "${BLUE}[3/7]${NC} Configurando variables de entorno..."

BACKUP_FILE="$HOME/.bashrc.backup.$(date +%Y%m%d_%H%M%S)"
cp ~/.bashrc "$BACKUP_FILE" 2>/dev/null || true
echo -e "${GREEN}✓${NC} Backup creado: ${BACKUP_FILE}"

# Añadir configuración si no existe
if ! grep -q "DESKTOP_TUI_PLATFORM" ~/.bashrc 2>/dev/null; then
    cat >> ~/.bashrc << 'EOF'

# ═══════════════════════════════════════════
# Desktop TUI - Optimizaciones para Termux
# ═══════════════════════════════════════════

export TERM=xterm-256color
export COLORTERM=truecolor
export DESKTOP_TUI_PLATFORM=termux
export LC_ALL=C.UTF-8
export LANG=C.UTF-8

# Habilitar soporte UTF-8 completo
export TERMUX_UTF8_FORCE=1

# Alias útiles para Desktop TUI
alias dtui='cd ~/desktop-tui 2>/dev/null && ./build/desktop-tui'
alias dtui-build='cd ~/desktop-tui && mkdir -p build && cd build && cmake .. -DPLATFORM=termux -DCMAKE_BUILD_TYPE=Release && cmake --build . -j$(nproc)'
alias dtui-clean='cd ~/desktop-tui && rm -rf build && mkdir build'
alias dtui-test='cd ~/desktop-tui/build && ctest --output-on-failure'

# Función para verificar capacidades del terminal
dtui-check() {
    echo "╔══════════════════════════════════════════╗"
    echo "║  Desktop TUI - Verificación de Sistema  ║"
    echo "╚══════════════════════════════════════════╝"
    echo ""
    echo "TERM: $TERM"
    echo "COLORTERM: $COLORTERM"
    echo "TERMUX_VERSION: ${TERMUX_VERSION:-NO DETECTADO}"
    echo "Arquitectura: $(uname -m)"
    echo "Android API Level: $(getprop ro.build.version.sdk 2>/dev/null || echo 'N/A')"
    echo ""
    
    # Test de colores TrueColor
    echo -e "Prueba TrueColor: \x1b[38;2;255;100;100m████████\x1b[0m \x1b[38;2;100;255;100m████████\x1b[0m \x1b[38;2;100;100;255m████████\x1b[0m"
    echo ""
    
    # Test de caracteres Unicode
    echo "Caracteres Unicode: ┌─┬─┐ │ │ │ ├─┼─┤ │ │ │ └─┴─┘"
    echo "Braille: ⠁⠂⠃⠄⠅⠆⠇⠈"
    echo ""
    
    # Verificar compilador
    if command -v clang++ &> /dev/null; then
        echo "Compilador: $(clang++ --version | head -n1)"
    else
        echo -e "${RED}❌ Compilador no encontrado${NC}"
    fi
}

# Fin de configuración Desktop TUI
EOF
    echo -e "${GREEN}✓${NC} Variables de entorno añadidas a ~/.bashrc"
else
    echo -e "${YELLOW}⚠️${NC} Las variables ya existen en ~/.bashrc"
fi

# 4. Configurar teclado extendido
echo -e "${BLUE}[4/7]${NC} Configurando teclado extendido..."

mkdir -p ~/.termux

cat > ~/.termux/termux.properties << 'EOF'
# ═══════════════════════════════════════════
# Configuración optimizada para Desktop TUI
# ═══════════════════════════════════════════

# Usar workaround para fullscreen
use-fullscreen-workaround=true

# Forzar input basado en caracteres (mejor compatibilidad)
enforce-char-based-input=true

# Permitir apps externas (para integración futura)
allow-external-apps=true

# Intentar modo fullscreen
fullscreen=true

# Mostrar teclas extra por defecto
show-extra-keys=true

# Layout de teclas extra optimizado para Desktop TUI
# Fila 1: Modificadores y símbolos especiales
# Fila 2: Navegación y edición
# Fila 3: Teclas de función
extra-keys=[[ESC, TAB, CTRL, ALT, {key: '-', popup: '|'}, {key: '/', popup: '\\'}, {key: '~', popup: '^'}],
            [UP, {key: LEFT, popup: HOME}, {key: DOWN, popup: END}, {key: RIGHT, popup: INSERT}, DELETE, BACKSPACE],
            [F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12]]

# Configurar volumen como teclas adicionales (opcional)
# volume-keys-up=PAGE_UP
# volume-keys-down=PAGE_DOWN

# Ajustar comportamiento del cursor
cursor-style=block
cursor-blink=false

# Mejorar renderizado de fuentes
font-size=14
use-black-background=true
EOF

echo -e "${GREEN}✓${NC} Configuración de teclado guardada en ~/.termux/termux.properties"

# 5. Recargar configuración
echo -e "${BLUE}[5/7]${NC} Aplicando cambios..."

# Intentar recargar settings de Termux
if command -v termux-reload-settings &> /dev/null; then
    termux-reload-settings
    echo -e "${GREEN}✓${NC} Configuración de Termux recargada"
else
    echo -e "${YELLOW}⚠️${NC} termux-reload-settings no disponible, reinicia manualmente"
fi

# 6. Verificar capacidades
echo -e "${BLUE}[6/7]${NC} Verificando capacidades del sistema..."

# Crear programa de test rápido
cat > /tmp/dtui_test.cpp << 'CPPEOF'
#include <iostream>
#include <cstdlib>
#include <cstring>

int main() {
    std::cout << "═══ VERIFICACIÓN DE CAPACIDADES ═══" << std::endl;
    std::cout << std::endl;
    
    // Variables de entorno
    const char* term = getenv("TERM");
    const char* colorterm = getenv("COLORTERM");
    const char* termux_ver = getenv("TERMUX_VERSION");
    const char* home = getenv("HOME");
    
    std::cout << "TERM: " << (term ? term : "NO SET") << std::endl;
    std::cout << "COLORTERM: " << (colorterm ? colorterm : "NO SET") << std::endl;
    std::cout << "TERMUX_VERSION: " << (termux_ver ? termux_ver : "NOT TERMUX") << std::endl;
    std::cout << "HOME: " << (home ? home : "NO SET") << std::endl;
    std::cout << std::endl;
    
    // Detectar Termux
    bool is_termux = false;
    if (termux_ver) is_termux = true;
    if (home && strstr(home, "/com.termux")) is_termux = true;
    
    if (is_termux) {
        std::cout << "✓ Entorno Termux detectado" << std::endl;
    } else {
        std::cout << "⚠ No se detectó Termux" << std::endl;
    }
    
    // Test TrueColor
    std::cout << std::endl;
    std::cout << "Prueba TrueColor:" << std::endl;
    std::cout << "\x1b[38;2;255;100;100m  Rojo RGB  \x1b[0m";
    std::cout << "\x1b[38;2;100;255;100m  Verde RGB \x1b[0m";
    std::cout << "\x1b[38;2;100;100;255m  Azul RGB  \x1b[0m" << std::endl;
    
    // Test 256 colores
    std::cout << std::endl;
    std::cout << "Prueba 256 colores:" << std::endl;
    for (int i = 0; i < 6; i++) {
        std::cout << "\x1b[48;5;" << (16 + i * 36) << "m  ";
    }
    std::cout << "\x1b[0m" << std::endl;
    
    // Test Unicode
    std::cout << std::endl;
    std::cout << "Caracteres Unicode:" << std::endl;
    std::cout << "Box-drawing: ┌─┬─┐ │ ├─┼─┤ └─┴─┘" << std::endl;
    std::cout << "Braille: ⠁⠂⠃⠄⠅⠆⠇⠈" << std::endl;
    std::cout << "Símbolos: ★ ✓ ✗ → ← ↑ ↓ ◆ ■ ●" << std::endl;
    
    std::cout << std::endl;
    std::cout << "═══ VERIFICACIÓN COMPLETADA ═══" << std::endl;
    
    return 0;
}
CPPEOF

# Compilar y ejecutar test
if g++ -std=c++17 -o /tmp/dtui_test /tmp/dtui_test.cpp 2>/dev/null; then
    /tmp/dtui_test
    echo -e "${GREEN}✓${NC} Test de capacidades completado"
else
    echo -e "${YELLOW}⚠️${NC} No se pudo compilar el test (esto es normal si es la primera vez)"
fi

# 7. Mensaje final
echo -e "${BLUE}[7/7]${NC} Finalizando configuración..."

echo ""
echo -e "${GREEN}╔════════════════════════════════════════════╗${NC}"
echo -e "${GREEN}║     ✅ ¡Configuración completada!          ║${NC}"
echo -e "${GREEN}╚════════════════════════════════════════════╝${NC}"
echo ""
echo -e "${CYAN}📋 Próximos pasos:${NC}"
echo ""
echo -e "  ${YELLOW}1.${NC} Reinicia Termux completamente"
echo -e "     (Desliza desde el borde para cerrar y vuelve a abrir)"
echo ""
echo -e "  ${YELLOW}2.${NC} Aplica los cambios:"
echo -e "     ${CYAN}source ~/.bashrc${NC}"
echo ""
echo -e "  ${YELLOW}3.${NC} Verifica la configuración:"
echo -e "     ${CYAN}dtui-check${NC}"
echo ""
echo -e "  ${YELLOW}4.${NC} Clona y compila Desktop TUI:"
echo -e "     ${CYAN}git clone https://github.com/tu-usuario/desktop-tui.git${NC}"
echo -e "     ${CYAN}cd desktop-tui${NC}"
echo -e "     ${CYAN}dtui-build${NC}"
echo ""
echo -e "  ${YELLOW}5.${NC} Ejecuta la demo:"
echo -e "     ${CYAN}dtui${NC}"
echo ""
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}💡 Comandos rápidos disponibles:${NC}"
echo -e "  • ${CYAN}dtui${NC} - Ejecutar Desktop TUI"
echo -e "  • ${CYAN}dtui-build${NC} - Compilar proyecto"
echo -e "  • ${CYAN}dtui-clean${NC} - Limpiar build"
echo -e "  • ${CYAN}dtui-test${NC} - Ejecutar tests"
echo -e "  • ${CYAN}dtui-check${NC} - Verificar sistema"
echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "${GREEN}¡Disfruta de Desktop TUI en tu dispositivo Android! 🎉${NC}"
echo ""

# Cleanup
rm -f /tmp/dtui_test.cpp /tmp/dtui_test 2>/dev/null || true

exit 0
