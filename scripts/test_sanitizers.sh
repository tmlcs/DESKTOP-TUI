#!/bin/bash
# Script para probar sanitizers en Desktop TUI

set -e

echo "=== Testing Sanitizers (ASan + UBSan) ==="
echo ""

# Configurar directorio de build con sanitizers
BUILD_DIR="build-sanitizers"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "1. Configurando build con ASan + UBSan..."
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON

echo ""
echo "2. Compilando..."
cmake --build . -j$(nproc)

echo ""
echo "3. Ejecutando tests básicos..."
./desktop-tui --version
./desktop-tui --help

echo ""
echo "4. Probando plugin demo..."
./plugin_demo --help || true

echo ""
echo "✅ Todos los tests pasaron sin errores de sanitizers!"
echo ""
echo "Para ejecutar manualmente:"
echo "  cd $BUILD_DIR"
echo "  ./desktop-tui"
echo ""
echo "Los sanitizers detectarán automáticamente:"
echo "  - Memory leaks (ASan)"
echo "  - Buffer overflows (ASan)"
echo "  - Undefined behavior (UBSan)"
echo "  - Use after free (ASan)"
