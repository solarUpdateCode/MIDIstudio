@echo off
title MIDIstudio Local Server Setup
echo Setting up local MuScriptor transcription server...
where uv >nul 2>&1
if %errorlevel% neq 0 (
    python -m pip install --upgrade uv
)
echo Pre-caching MuScriptor local server...
uvx muscriptor --help >nul 2>&1
echo Setup complete.
