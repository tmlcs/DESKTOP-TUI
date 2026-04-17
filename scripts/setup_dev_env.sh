#!/bin/bash
# Script de inicialización para desarrollo - Horizonte 1
# Este script configura el entorno de desarrollo con sanitizers y herramientas de calidad

set -e

echo "🚀 Iniciando configuración del entorno de desarrollo - Horizonte 1 (v0.3.1)"
echo "=========================================================================="

# Detectar sistema operativo
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
else
    echo "❌ Sistema operativo no soportado: $OSTYPE"
    exit 1
fi

echo "✅ Sistema operativo detectado: $OS"

# Crear directorios necesarios
echo "📁 Creando estructura de directorios..."
mkdir -p build-debug build-asan build-tidy
mkdir -p scripts
mkdir -p .github/workflows

# Instalar dependencias según SO
if [[ "$OS" == "linux" ]]; then
    echo "📦 Instalando dependencias para Linux..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update || true
        sudo apt-get install -y \
            g++ \
            clang \
            clang-format \
            clang-tidy \
            cmake \
            ninja-build \
            valgrind \
            doxygen \
            graphviz \
            git || true
        echo "✅ Dependencias instaladas (apt)"
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y \
            gcc-c++ \
            clang \
            clang-tools-extra \
            cmake \
            ninja-build \
            valgrind \
            doxygen \
            graphviz \
            git || true
        echo "✅ Dependencias instaladas (dnf)"
    fi
elif [[ "$OS" == "macos" ]]; then
    echo "📦 Instalando dependencias para macOS..."
    if command -v brew &> /dev/null; then
        brew install \
            gcc \
            llvm \
            cmake \
            ninja \
            valgrind \
            doxygen \
            graphviz \
            git || true
        echo "✅ Dependencias instaladas (brew)"
    else
        echo "⚠️ Homebrew no encontrado. Por favor instala las dependencias manualmente."
    fi
fi

# Configurar build con sanitizers
echo "🔧 Configurando build con AddressSanitizer y UndefinedBehaviorSanitizer..."
cd build-asan
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_ASAN=ON \
    -DENABLE_UBSAN=ON \
    -DENABLE_TESTS=ON \
    -DENABLE_PLUGINS=ON \
    -G Ninja
cd ..
echo "✅ Build ASan configurado en build-asan/"

# Configurar build para clang-tidy
echo "🔍 Configurando build para análisis estático..."
cd build-tidy
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DENABLE_TESTS=ON \
    -G Ninja
cd ..
echo "✅ Build tidy configurado en build-tidy/"

# Copiar compile_commands.json al root
if [[ -f "build-tidy/compile_commands.json" ]]; then
    cp build-tidy/compile_commands.json .
    echo "✅ compile_commands.json copiado al root del proyecto"
fi

# Crear build debug estándar
echo "🛠️ Configurando build debug estándar..."
cd build-debug
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DENABLE_TESTS=ON \
    -DENABLE_PLUGINS=ON \
    -G Ninja
cd ..
echo "✅ Build debug configurado en build-debug/"

# Verificar que todo esté correcto
echo ""
echo "📊 Resumen de la configuración:"
echo "================================"
echo "Builds disponibles:"
echo "  - build-debug/     : Build estándar con tests"
echo "  - build-asan/      : Build con sanitizers (ASan + UBSan)"
echo "  - build-tidy/      : Build para análisis estático"
echo ""
echo "Comandos útiles:"
echo "  cd build-debug && ninja && ctest --output-on-failure"
echo "  cd build-asan && ninja && ctest --output-on-failure"
echo "  cd build-tidy && ninja"
echo ""
echo "Análisis de código:"
echo "  clang-tidy src/**/*.cpp -p build-tidy"
echo "  clang-format --dry-run --Werror src/**/*.cpp include/**/*.hpp"
echo ""
echo "✅ ¡Configuración completada exitosamente!"
echo ""
echo "📋 Próximos pasos:"
echo "  1. Revisar los issues en roadmap/horizon1_github_issues.md"
echo "  2. Comenzar con [ARCH-01] Auditoría de Headers"
echo "  3. Ejecutar tests con sanitizers: cd build-asan && ctest"
echo ""
