#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT_DIR"

if [ -z "${PSPDEV:-}" ] && [ -d "$ROOT_DIR/.toolchain/pspdev" ]; then
    export PSPDEV="$ROOT_DIR/.toolchain/pspdev"
fi

if [ -z "${PSPDEV:-}" ] || [ ! -x "$PSPDEV/bin/psp-gcc" ]; then
    echo "Error: PSPDEV not found. Please set the PSPDEV environment variable." >&2
    exit 1
fi

export PATH="$PSPDEV/bin:$PATH"

rm -rf "$ROOT_DIR/dist/PSP/GAME/MZWAVE"
rm -f "$ROOT_DIR/mzwave.elf" "$ROOT_DIR/mzwave.prx"

python3 "$ROOT_DIR/tools/gen_assets.py"
make clean
make -j4

mkdir -p "$ROOT_DIR/dist/PSP/GAME/ERENCYK/playlists"
cp -f "$ROOT_DIR/EBOOT.PBP" "$ROOT_DIR/dist/PSP/GAME/ERENCYK/EBOOT.PBP"

echo ""
echo "=========================================================="
echo " BUILD COMPLETE: dist/PSP/GAME/ERENCYK/EBOOT.PBP is ready!"
echo "=========================================================="
ls -lh "$ROOT_DIR/dist/PSP/GAME/ERENCYK/EBOOT.PBP"
