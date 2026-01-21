#!/bin/bash
# Rodmin OS - App Build System
# Handles application-specific compilation and bundling

source "$(dirname "${BASH_SOURCE[0]}")/setup_env.sh"

APP_NAME=$1
APP_DIR="$PROJECT_ROOT/apps/$APP_NAME"

if [[ ! -d "$APP_DIR" ]]; then
    echo "Error: Application $APP_NAME not found in $PROJECT_ROOT/apps"
    exit 1
fi

echo "Building $APP_NAME..."

# Compile application
$CC -I"$SDK_INCDIR" -L"$SDK_LIBDIR" \
    "$APP_DIR/main.c" -o "$PROJECT_ROOT/build/apps/$APP_NAME.bin" -lrodmin

# Bundle assets if present
if [[ -d "$APP_DIR/assets" ]]; then
    echo "Bundling assets for $APP_NAME..."
    cp -r "$APP_DIR/assets/"* "$ASSETS_DIR/"
fi

echo "Build complete: $PROJECT_ROOT/build/apps/$APP_NAME.bin"
