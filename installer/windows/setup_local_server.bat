@echo off
title MIDIstudio Local Bridge Setup
echo Installing lightweight MIDIstudio local server dependencies (no HuggingFace login required)...
python -m pip install --quiet --upgrade fastapi uvicorn httpx python-multipart
echo Setup complete.
