#!/usr/bin/env bash
set -euo pipefail
if ! command -v uvx >/dev/null 2>&1; then
    python3 -m pip install --user --upgrade uv || true
    export PATH="$HOME/.local/bin:$PATH"
fi
exec uvx muscriptor serve --port 8222
