@echo off
title MIDIstudio Local Transcription Bridge (Port 8222)
cd /d "%~dp0"
python -c "import fastapi, uvicorn, httpx" >nul 2>&1
if %errorlevel% neq 0 (
    python -m pip install --quiet fastapi uvicorn httpx python-multipart
)
python local_bridge.py
