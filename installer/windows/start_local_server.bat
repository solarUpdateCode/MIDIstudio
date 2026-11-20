@echo off
title MIDIstudio Local Transcription Server (Port 8222)
where uvx >nul 2>&1
if %errorlevel% neq 0 (
    python -m pip install --upgrade uv
)
uvx --torch-backend=cu128 muscriptor serve --port 8222
if %errorlevel% neq 0 (
    uvx muscriptor serve --port 8222
)
