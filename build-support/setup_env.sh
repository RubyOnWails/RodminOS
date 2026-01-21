#!/bin/bash
# Rodmin OS Build Support - Dynamic Environment Setup
# Detects OS and paths automatically

PROJECT_ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

# Convert to Windows style path if in MSYS/WSL but building for Windows toolchain
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    export WIN_PROJECT_ROOT=$(cygpath -w "$PROJECT_ROOT")
    echo "Running on Windows (MSYS/Cygwin shell)"
else
    export WIN_PROJECT_ROOT="$PROJECT_ROOT"
    echo "Running on Unix-like system"
fi

export SDK_INCDIR="$PROJECT_ROOT/sdk/include"
export SDK_LIBDIR="$PROJECT_ROOT/sdk/lib"
export ASSETS_DIR="$PROJECT_ROOT/assets"

# Toolchain check
if [[ -z "$CC" ]]; then
    if command -v x86_64-elf-gcc &> /dev/null; then
        export CC="x86_64-elf-gcc"
    else
        export CC="gcc"
    fi
fi

echo "Environment initialized."
echo "Root: $PROJECT_ROOT"
echo "Compiler: $CC"
