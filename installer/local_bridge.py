"""Lightweight local SSE bridge for MIDIstudio (no HuggingFace token or GPU required)."""
from fastapi import FastAPI, File, Form, UploadFile
from fastapi.responses import Response, StreamingResponse
import httpx
import uvicorn

REMOTE = "https://muscriptor.kyutai.org"
HEADERS = {
    "Origin": REMOTE,
    "Referer": REMOTE + "/",
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 Chrome/131.0.0.0 Safari/537.36",
}

app = FastAPI(title="MIDIstudio Local Bridge")

@app.get("/")
async def health():
    return {"status": "ok"}

@app.post("/transcribe")
async def transcribe_sse(file: UploadFile = File(...), instruments: str = Form(default="")):
    raw = await file.read()
    fname = file.filename or "audio.wav"
    ctype = file.content_type or "audio/wav"
    data = {"instruments": instruments} if instruments.strip() else {}

    async def gen():
        async with httpx.AsyncClient(timeout=None, follow_redirects=True) as client:
            async with client.stream(
                "POST",
                REMOTE + "/transcribe",
                files={"file": (fname, raw, ctype)},
                data=data,
                headers={**HEADERS, "Accept": "text/event-stream"},
            ) as resp:
                async for chunk in resp.aiter_bytes():
                    if chunk:
                        yield chunk

    return StreamingResponse(gen(), media_type="text/event-stream")

@app.post("/transcribe/midi")
async def transcribe_midi(file: UploadFile = File(...), instruments: str = Form(default="")):
    raw = await file.read()
    fname = file.filename or "audio.wav"
    ctype = file.content_type or "audio/wav"
    data = {"instruments": instruments} if instruments.strip() else {}
    async with httpx.AsyncClient(timeout=300.0, follow_redirects=True) as client:
        resp = await client.post(
            REMOTE + "/transcribe/midi",
            files={"file": (fname, raw, ctype)},
            data=data,
            headers=HEADERS,
        )
        return Response(content=resp.content, status_code=resp.status_code, media_type="audio/midi")

@app.post("/sheets")
async def sheets(file: UploadFile = File(...)):
    raw = await file.read()
    async with httpx.AsyncClient(timeout=120.0, follow_redirects=True) as client:
        resp = await client.post(
            REMOTE + "/sheets",
            files={"file": (file.filename or "score.mid", raw, "audio/midi")},
            headers=HEADERS,
        )
        return Response(content=resp.content, status_code=resp.status_code, media_type="application/zip")

if __name__ == "__main__":
    uvicorn.run(app, host="127.0.0.1", port=8222)
