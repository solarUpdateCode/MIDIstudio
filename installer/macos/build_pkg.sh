#!/usr/bin/env bash
set -euo pipefail
VERSION="1.0.0"
ARTEFACTS="build/MIDIstudio_artefacts/Release"
STAGE="installer/macos/stage"
OUT="MIDIstudio-${VERSION}-macOS-Installer.pkg"
rm -rf "$STAGE"
mkdir -p "$STAGE/Applications" "$STAGE/Library/Audio/Plug-Ins/VST3" "$STAGE/Library/Audio/Plug-Ins/Components"
cp -R "${ARTEFACTS}/Standalone/MIDIstudio.app" "$STAGE/Applications/"
cp -R "${ARTEFACTS}/VST3/MIDIstudio.vst3" "$STAGE/Library/Audio/Plug-Ins/VST3/"
cp -R "${ARTEFACTS}/AU/MIDIstudio.component" "$STAGE/Library/Audio/Plug-Ins/Components/"
pkgbuild --root "$STAGE" --identifier "com.solaraudio.midistudio" --version "$VERSION" --install-location "/" "$OUT"
echo "Built macOS Universal Installer: $OUT"
