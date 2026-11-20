# MIDIstudio by solaraudio

MIDIstudio is an audio-to-MIDI studio and pattern workflow application by **solaraudio**, built on [muscriptor](https://github.com/muscriptor/muscriptor) and [muscriptor.kyutai.org](https://muscriptor.kyutai.org/).

## Features

- **Brushed-Gold Hardware UI**: Tactile studio-inspired interface designed for fast audio and MIDI workflows.
- **DAW-Ready Individual Pattern Layer Splitter**: Export separated, production-ready MIDI pattern layers for immediate DAW import.
- **GM Drum Sub-Layer Splitting**: Split General MIDI drum tracks into individual drum sub-layers.
- **Zero-Aligned Sync Headers**: Maintain sample-accurate alignment across exported tracks with zero-aligned sync headers.
- **Spacebar Playback**: Instant transport control with Spacebar playback.
- **Editable Key / Scale / Fixed Tempo**: Adjust musical key, scale, and fixed tempo directly within the studio.
- **Quick MIDI Editor**: Inspect and refine MIDI notes inline without leaving the workflow.
- **Chord Progression Track**: View and work with harmonic chord progressions alongside your patterns.
- **Demucs Stem Bundle Support**: Optional stem separation powered by Demucs for multi-stem audio-to-MIDI workflows.

## Quick Start

Install dependencies using `uv`:

```bash
uv sync
```

Run MIDIstudio on port `8765`:

```bash
uv run midistudio --port 8765
```
