set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
OUT_DIR="$ROOT_DIR/dist"
ASCII_DIR="$ROOT_DIR/assets/intro"
SHELL_FILE="$ROOT_DIR/assets/web/web-shell.html"
FONT_SOURCE="$ROOT_DIR/assets/fonts/NanumGothicCoding.ttf"
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR"

if [ ! -d "$ASCII_DIR" ]; then
  printf 'Missing ASCII intro directory: %s\n' "$ASCII_DIR" >&2
  exit 1
fi

if ! command -v em++ >/dev/null 2>&1; then
  printf 'em++ is required. Install and activate Emscripten first.\n' >&2
  exit 1
fi

em++ "$ROOT_DIR/main_web.cpp" \
  -std=c++17 \
  -O2 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sNO_EXIT_RUNTIME=1 \
  -sASYNCIFY=1 \
  -sENVIRONMENT=web \
  -sEXPORTED_RUNTIME_METHODS=ccall,cwrap \
  -sEXPORTED_FUNCTIONS=_main \
  -sFULL_ES3=1 \
  -sFORCE_FILESYSTEM=1 \
  --shell-file "$SHELL_FILE" \
  --preload-file "$ASCII_DIR@/assets/intro" \
  --preload-file "$ROOT_DIR/assets/logo@/assets/logo" \
  -o "$OUT_DIR/index.html"

mkdir -p "$OUT_DIR/assets/fonts"
cp "$FONT_SOURCE" "$OUT_DIR/assets/fonts/NanumGothicCoding.ttf"

printf 'Built web app at %s\n' "$OUT_DIR/index.html"
