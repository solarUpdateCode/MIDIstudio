#!/usr/bin/env bash
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
python3 -c "import fastapi, uvicorn, httpx" >/dev/null 2>&1 || python3 -m pip install --user --quiet fastapi uvicorn httpx python-multipart
exec python3 "$DIR/local_bridge.py"
