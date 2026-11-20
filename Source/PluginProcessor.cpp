#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <thread>

namespace {
    const juce::StringArray kMeterChoices { "4/4", "3/4", "6/8", "2/4", "5/4", "7/8" };
    const juce::StringArray kKeyChoices   { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    const juce::StringArray kScaleChoices { "Minor", "Major", "Dorian", "Mixolydian", "Harmonic Minor", "Pentatonic Minor" };
    const juce::Colour kPalette[] = { juce::Colour (0xffffb84d), juce::Colour (0xffe67e35), juce::Colour (0xfff0c46c), juce::Colour (0xfff26d52), juce::Colour (0xffffd98a), juce::Colour (0xff78c6a3) };
}

const juce::StringArray& MIDIstudioAudioProcessor::getMuscriptorInstruments()
{
    static const juce::StringArray kInst {
        "acoustic piano", "electric piano", "chromatic percussion", "organ",
        "acoustic guitar", "clean electric guitar", "distorted electric guitar",
        "acoustic bass", "electric bass", "violin", "viola", "cello", "contrabass",
        "orchestral harp", "timpani", "string ensemble", "synth strings", "voice",
        "orchestra hit", "trumpet", "trombone", "tuba", "french horn", "brass section",
        "soprano & alto sax", "tenor sax", "baritone sax", "oboe", "english horn",
        "bassoon", "clarinet", "flutes", "synth lead", "synth pad", "drums"
    };
    return kInst;
}

MIDIstudioAudioProcessor::MIDIstudioAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MIDIstudioState", createParameterLayout())
{
    voices.resize (32);
    clearSessionToCleanRoll();
}

juce::AudioProcessorValueTreeState::ParameterLayout MIDIstudioAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("mix", "Original-MIDI Mix", 0.0f, 100.0f, 100.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("stereoSplit", "Stereo Audit Split", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("quantizeStrength", "Quantize Strength", 0.0f, 100.0f, 50.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool>  ("fixedTempo", "Fixed Tempo Mode", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> ("fixedBpm", "Fixed Tempo BPM", juce::NormalisableRange<float> (30.0f, 300.0f, 0.5f), 122.5f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("meterIdx", "Meter", kMeterChoices, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("keyRoot", "Key Root", kKeyChoices, 4));
    p.push_back (std::make_unique<juce::AudioParameterChoice> ("scaleIdx", "Scale", kScaleChoices, 0));
    return { p.begin(), p.end() };
}

void MIDIstudioAudioProcessor::clearSessionToCleanRoll()
{
    const juce::ScopedLock sl (stateLock);
    isPlaying.store (false);
    playheadSec.store (0.0);
    tracks.clear();
    chords.clear();
    loadedAudioBuffer.setSize (0, 0);
    loadedAudioFile = juce::File{};
    sessionName = "Session_01";
    statusMessage = "Clean Roll - Ready";
    barOffsetSec.store (0.0);
}

void MIDIstudioAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    for (auto& v : voices) v.active = false;
}

bool MIDIstudioAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

double MIDIstudioAudioProcessor::getEffectiveBpm() const
{
    return apvts.getRawParameterValue ("fixedTempo")->load() > 0.5f
           ? static_cast<double> (apvts.getRawParameterValue ("fixedBpm")->load())
           : detectedBpm.load();
}

int MIDIstudioAudioProcessor::getBeatsPerBar() const { return getMeterString().upToFirstOccurrenceOf ("/", false, false).getIntValue(); }
juce::String MIDIstudioAudioProcessor::getMeterString() const { return kMeterChoices[juce::jlimit (0, kMeterChoices.size() - 1, static_cast<int> (apvts.getRawParameterValue ("meterIdx")->load()))]; }
juce::String MIDIstudioAudioProcessor::getKeyRootString() const { return kKeyChoices[juce::jlimit (0, kKeyChoices.size() - 1, static_cast<int> (apvts.getRawParameterValue ("keyRoot")->load()))]; }
juce::String MIDIstudioAudioProcessor::getScaleString() const { return kScaleChoices[juce::jlimit (0, kScaleChoices.size() - 1, static_cast<int> (apvts.getRawParameterValue ("scaleIdx")->load()))]; }

int MIDIstudioAudioProcessor::getProcessedVelocity (const InstrumentTrack& trk, int rawVel) const
{
    float v = static_cast<float> (rawVel) * (trk.velScale / 100.0f);
    if (trk.velComp > 0.0f) { float c = juce::jlimit (0.0f, 1.0f, trk.velComp / 100.0f); v = v * (1.0f - c) + 96.0f * c; }
    return juce::jlimit (1, 127, juce::roundToInt (v));
}

bool MIDIstudioAudioProcessor::isTrackAudible (const InstrumentTrack& trk) const
{
    for (const auto& t : tracks) if (t.solo) return trk.solo;
    return !trk.mute;
}

void MIDIstudioAudioProcessor::triggerVoice (const InstrumentTrack& trk, const MidiNoteItem& note)
{
    for (auto& v : voices) {
        if (!v.active) {
            v.active = true; v.pitch = note.pitch; v.isDrum = trk.isDrum; v.phase = 0.0;
            v.freq = trk.isDrum ? (note.pitch <= 36 ? 68.0 : 900.0) : 440.0 * std::pow (2.0, (note.pitch - 69) / 12.0);
            v.gain = (getProcessedVelocity (trk, note.velocity) / 127.0f) * 0.16f;
            v.totalSamples = std::max (256, static_cast<int> (juce::jlimit (0.06, 2.5, note.durSec) * currentSampleRate));
            v.samplesRemaining = v.totalSamples;
            return;
        }
    }
}

void MIDIstudioAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples(), numOutCh = buffer.getNumChannels();
    juce::AudioBuffer<float> origCopy; origCopy.makeCopyOf (buffer); buffer.clear();
    const float mix = apvts.getRawParameterValue ("mix")->load() / 100.0f;
    const bool stereoSplit = apvts.getRawParameterValue ("stereoSplit")->load() > 0.5f;

    if (isPlaying.load()) {
        const double blockDur = static_cast<double> (numSamples) / currentSampleRate;
        const double t0 = playheadSec.load(), t1 = t0 + blockDur;
        const double tempoRatio = detectedBpm.load() / std::max (20.0, getEffectiveBpm());
        const juce::ScopedTryLock sl (stateLock);
        if (sl.isLocked()) {
            for (size_t ti = 0; ti < tracks.size(); ++ti) {
                const auto& trk = tracks[ti];
                if (!isTrackAudible (trk)) continue;
                const int midiCh = trk.isDrum ? 10 : static_cast<int> ((ti % 15) + 1);
                for (const auto& n : trk.notes) {
                    const double ns = n.startSec * tempoRatio, ne = (n.startSec + n.durSec) * tempoRatio;
                    if (ns >= t0 && ns < t1) {
                        int sp = juce::jlimit (0, numSamples - 1, static_cast<int> ((ns - t0) * currentSampleRate));
                        midiMessages.addEvent (juce::MidiMessage::noteOn (midiCh, n.pitch, static_cast<juce::uint8> (getProcessedVelocity (trk, n.velocity))), sp);
                        triggerVoice (trk, n);
                    }
                    if (ne >= t0 && ne < t1) {
                        int sp = juce::jlimit (0, numSamples - 1, static_cast<int> ((ne - t0) * currentSampleRate));
                        midiMessages.addEvent (juce::MidiMessage::noteOff (midiCh, n.pitch), sp);
                    }
                }
            }
        }
        playheadSec.store (t1 >= durationSec.load() ? 0.0 : t1);
    }

    const double curSec = playheadSec.load();
    for (int i = 0; i < numSamples; ++i) {
        float synthSample = 0.0f;
        for (auto& v : voices) {
            if (!v.active) continue;
            float env = static_cast<float> (v.samplesRemaining) / static_cast<float> (v.totalSamples);
            if (v.isDrum && v.pitch > 36) synthSample += (rng.nextFloat() * 2.0f - 1.0f) * v.gain * env;
            else { synthSample += std::sin (static_cast<float> (v.phase)) * v.gain * env; v.phase += juce::MathConstants<double>::twoPi * v.freq / currentSampleRate; }
            if (--v.samplesRemaining <= 0) v.active = false;
        }
        float origL = numOutCh > 0 ? origCopy.getSample (0, i) : 0.0f;
        float origR = numOutCh > 1 ? origCopy.getSample (1, i) : origL;
        if (isPlaying.load() && loadedAudioBuffer.getNumSamples() > 0) {
            int srcIdx = static_cast<int> (curSec * loadedAudioSampleRate) + i;
            if (srcIdx >= 0 && srcIdx < loadedAudioBuffer.getNumSamples()) {
                origL += loadedAudioBuffer.getSample (0, srcIdx);
                origR += loadedAudioBuffer.getNumChannels() > 1 ? loadedAudioBuffer.getSample (1, srcIdx) : origL;
            }
        }
        if (stereoSplit && numOutCh >= 2) { buffer.setSample (0, i, origL * 0.85f); buffer.setSample (1, i, synthSample * 0.95f); }
        else {
            if (numOutCh > 0) buffer.setSample (0, i, origL * (1.0f - mix) + synthSample * mix);
            if (numOutCh > 1) buffer.setSample (1, i, origR * (1.0f - mix) + synthSample * mix);
        }
    }
}

bool MIDIstudioAudioProcessor::loadMidiFromMemory (const juce::MemoryBlock& midiBytes, const juce::String& title)
{
    juce::MemoryInputStream mis (midiBytes, false);
    juce::MidiFile mf;
    if (!mf.readFrom (mis)) return false;
    mf.convertTimestampTicksToSeconds();
    const juce::ScopedLock sl (stateLock);
    tracks.clear();
    if (title.isNotEmpty()) sessionName = title;
    double maxSec = 8.0;
    for (int ti = 0; ti < mf.getNumTracks(); ++ti) {
        const auto* seq = mf.getTrack (ti); if (!seq) continue;
        InstrumentTrack trk; trk.id = "trk_" + juce::String (ti); trk.name = "track " + juce::String (ti + 1); trk.colour = kPalette[ti % 6];
        for (int ei = 0; ei < seq->getNumEvents(); ++ei) {
            const auto& ev = *seq->getEventPointer (ei); const auto& msg = ev.message;
            if (msg.isTrackNameEvent()) trk.name = msg.getTextFromTextMetaEvent().toLowerCase();
            else if (msg.isProgramChange()) trk.program = msg.getProgramChangeNumber();
            else if (msg.isTempoMetaEvent() && msg.getTempoSecondsPerQuarterNote() > 0.0) detectedBpm.store (std::round ((60.0 / msg.getTempoSecondsPerQuarterNote()) * 2.0) / 2.0);
            else if (msg.isNoteOn()) {
                if (msg.getChannel() == 10) trk.isDrum = true;
                double endT = ev.noteOffObject != nullptr ? ev.noteOffObject->message.getTimeStamp() : msg.getTimeStamp() + 0.25;
                trk.notes.push_back ({ nextNoteId++, msg.getNoteNumber(), msg.getTimeStamp(), std::max (0.04, endT - msg.getTimeStamp()), msg.getVelocity() });
                maxSec = std::max (maxSec, endT + 0.5);
            }
        }
        if (!trk.notes.empty()) tracks.push_back (std::move (trk));
    }
    durationSec.store (maxSec);
    playheadSec.store (0.0);
    statusMessage = "Transcribed (" + juce::String (tracks.size()) + " tracks)";
    detectKeyAndChords();
    return true;
}

bool MIDIstudioAudioProcessor::loadAudioOrMidiFile (const juce::File& file)
{
    if (!file.existsAsFile()) return false;
    const auto ext = file.getFileExtension().toLowerCase();
    if (ext == ".mid" || ext == ".midi") {
        juce::MemoryBlock mb;
        if (!file.loadFileAsData (mb)) return false;
        return loadMidiFromMemory (mb, file.getFileNameWithoutExtension());
    }
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    if (auto reader = std::unique_ptr<juce::AudioFormatReader> (fm.createReaderFor (file))) {
        const juce::ScopedLock sl (stateLock);
        int nSamp = static_cast<int> (std::min<juce::int64> (reader->lengthInSamples, static_cast<juce::int64> (reader->sampleRate * 300.0)));
        loadedAudioBuffer.setSize (static_cast<int> (reader->numChannels), nSamp);
        reader->read (&loadedAudioBuffer, 0, nSamp, 0, true, true);
        loadedAudioSampleRate = reader->sampleRate;
        loadedAudioFile = file;
        sessionName = file.getFileNameWithoutExtension();
        durationSec.store (std::max (4.0, static_cast<double> (nSamp) / reader->sampleRate));
        playheadSec.store (0.0);
        statusMessage = "Audio ready - click START TRANSCRIBE";
        return true;
    }
    loadedAudioFile = file;
    sessionName = file.getFileNameWithoutExtension();
    statusMessage = "File selected - click START TRANSCRIBE";
    return true;
}

void MIDIstudioAudioProcessor::startMuscriptorTranscription (std::function<void(bool, juce::String)> onFinished)
{
    if (!loadedAudioFile.existsAsFile()) {
        if (onFinished) onFinished (false, "Load an audio file first");
        return;
    }
    if (isTranscribing.exchange (true)) return;

    const auto audioFile = loadedAudioFile;
    const auto whitelist = strictInstrumentWhitelist.joinIntoString (",");
    {
        const juce::ScopedLock sl (stateLock);
        statusMessage = "Transcribing via muscriptor.kyutai.org...";
    }

    std::thread ([this, audioFile, whitelist, onFinished = std::move (onFinished)]() mutable {
        juce::String endpoint = juce::String ("https://muscriptor.kyutai.org") + "/transcribe/midi";
        juce::URL url (endpoint);
        url = url.withFileToUpload ("file", audioFile, "application/octet-stream");
        if (whitelist.isNotEmpty()) url = url.withParameter ("instruments", whitelist);

        int statusCode = 0;
        auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                           .withConnectionTimeoutMs (240000)
                           .withStatusCode (&statusCode);

        bool ok = false;
        juce::String msg = "Could not reach muscriptor.kyutai.org";
        if (auto stream = url.createInputStream (options)) {
            juce::MemoryBlock responseData;
            stream->readEntireStreamAsMemoryBlock (responseData);
            if (statusCode == 200 && responseData.getSize() > 14) {
                ok = loadMidiFromMemory (responseData, audioFile.getFileNameWithoutExtension());
                msg = ok ? "Transcribed via muscriptor.kyutai.org" : "Invalid MIDI returned by server";
            } else {
                msg = "Server HTTP " + juce::String (statusCode) + " - try a shorter audio clip";
            }
        }
        isTranscribing.store (false);
        {
            const juce::ScopedLock sl (stateLock);
            statusMessage = msg;
        }
        juce::MessageManager::callAsync ([ok, msg, cb = std::move (onFinished)]() { if (cb) cb (ok, msg); });
    }).detach();
}

void MIDIstudioAudioProcessor::detectKeyAndChords()
{
    const juce::ScopedLock sl (stateLock); chords.clear(); if (tracks.empty()) return;
    static constexpr double majP[12] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    static constexpr double minP[12] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
    double hist[12] = {};
    for (const auto& t : tracks) if (!t.isDrum && !t.mute) for (const auto& n : t.notes) hist[n.pitch % 12] += std::max (0.05, n.durSec);
    double sum = 0.0; for (double v : hist) sum += v;
    if (sum > 0.0) {
        double best = -1e9; int bestR = 4, bestS = 0;
        for (int r = 0; r < 12; ++r) {
            double sMaj = 0.0, sMin = 0.0;
            for (int i = 0; i < 12; ++i) { sMaj += hist[(r + i) % 12] * majP[i]; sMin += hist[(r + i) % 12] * minP[i]; }
            if (sMaj > best) { best = sMaj; bestR = r; bestS = 1; }
            if (sMin > best) { best = sMin; bestR = r; bestS = 0; }
        }
        if (auto* p = apvts.getParameter ("keyRoot")) p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (bestR)));
        if (auto* p = apvts.getParameter ("scaleIdx")) p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (bestS)));
    }
    const double barSec = (60.0 / std::max (20.0, getEffectiveBpm())) * getBeatsPerBar();
    const int numBars = std::max (1, static_cast<int> (std::ceil (durationSec.load() / barSec)));
    for (int b = 0; b < numBars; ++b) {
        double t0 = barOffsetSec.load() + b * barSec, t1 = t0 + barSec, pcs[12] = {};
        for (const auto& t : tracks) if (!t.isDrum) for (const auto& n : t.notes)
            if (n.startSec + n.durSec > t0 && n.startSec < t1) pcs[n.pitch % 12] += std::max (0.05, std::min (n.startSec + n.durSec, t1) - std::max (n.startSec, t0));
        double bestV = -1.0; juce::String sym = getKeyRootString(); std::vector<int> notes { 48, 51, 55 };
        for (int r = 0; r < 12; ++r) {
            double majV = pcs[r] * 1.4 + pcs[(r + 4) % 12] + pcs[(r + 7) % 12];
            double minV = pcs[r] * 1.4 + pcs[(r + 3) % 12] + pcs[(r + 7) % 12];
            if (majV > bestV) { bestV = majV; sym = kKeyChoices[r]; notes = { 48 + r, 52 + r, 55 + r }; }
            if (minV > bestV) { bestV = minV; sym = kKeyChoices[r] + "m"; notes = { 48 + r, 51 + r, 55 + r }; }
        }
        chords.push_back ({ b + 1, t0, barSec, sym, notes });
    }
}

void MIDIstudioAudioProcessor::removeGhostNotes()
{
    const juce::ScopedLock sl (stateLock);
    for (auto& t : tracks)
        t.notes.erase (std::remove_if (t.notes.begin(), t.notes.end(), [] (const MidiNoteItem& n) { return n.durSec < 0.08 || n.velocity < 45; }), t.notes.end());
}

void MIDIstudioAudioProcessor::transposeTrack (int trackIndex, int semitones)
{
    const juce::ScopedLock sl (stateLock);
    if (trackIndex < 0 || trackIndex >= static_cast<int> (tracks.size()) || tracks[trackIndex].isDrum) return;
    for (auto& n : tracks[trackIndex].notes) n.pitch = juce::jlimit (24, 96, n.pitch + semitones);
}

juce::String MIDIstudioAudioProcessor::formatLayerFilename (int index, const juce::String& label) const
{
    juce::String clean = label.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 _-").replaceCharacter (' ', '_');
    juce::StringArray parts;
    if (tplTrackNum) parts.add (juce::String::formatted ("%02d", index));
    parts.add (clean);
    if (tplBpm) parts.add (juce::String (juce::roundToInt (getEffectiveBpm())) + "BPM");
    if (tplKey) parts.add (getKeyRootString() + (getScaleString().containsIgnoreCase ("Minor") ? "m" : "Maj"));
    return parts.joinIntoString ("_") + ".mid";
}

juce::MidiFile MIDIstudioAudioProcessor::buildSingleTrackMidiFile (const InstrumentTrack& trk, int channelIndex) const
{
    const int ppq = 480;
    const double bpm = std::max (20.0, getEffectiveBpm());
    const int mpqn = juce::roundToInt (60000000.0 / bpm);
    const double qStep = (60.0 / bpm) / 4.0, qAmt = static_cast<double> (apvts.getRawParameterValue ("quantizeStrength")->load()) / 100.0;
    const double tempoRatio = detectedBpm.load() / bpm;
    juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq);
    juce::MidiMessageSequence meta;
    meta.addEvent (juce::MidiMessage::tempoMetaEvent (mpqn), 0.0);
    meta.addEvent (juce::MidiMessage::timeSignatureMetaEvent (getBeatsPerBar(), 4), 0.0);
    mf.addTrack (meta);
    juce::MidiMessageSequence seq;
    const int ch = trk.isDrum ? 10 : ((channelIndex % 15) + 1 == 10 ? 11 : (channelIndex % 15) + 1);
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, trk.name), 0.0);
    if (!trk.isDrum) seq.addEvent (juce::MidiMessage::programChange (ch, trk.program), 0.0);
    for (const auto& n : trk.notes) {
        double s = std::max (0.0, n.startSec * tempoRatio - barOffsetSec.load()), d = n.durSec * tempoRatio;
        if (qAmt > 0.0) s += (std::round (s / qStep) * qStep - s) * qAmt;
        double tOn = std::max (0.0, (s / (60.0 / bpm)) * ppq), tOff = std::max (tOn + 10.0, ((s + d) / (60.0 / bpm)) * ppq);
        seq.addEvent (juce::MidiMessage::noteOn (ch, n.pitch, static_cast<juce::uint8> (getProcessedVelocity (trk, n.velocity))), tOn);
        seq.addEvent (juce::MidiMessage::noteOff (ch, n.pitch), tOff);
    }
    seq.sort(); seq.updateMatchedPairs(); mf.addTrack (seq); return mf;
}

juce::File MIDIstudioAudioProcessor::writeTempDragMidiForTrack (int trackIndex) const
{
    const juce::ScopedLock sl (stateLock);
    if (trackIndex < 0 || trackIndex >= static_cast<int> (tracks.size())) return {};
    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("MIDIstudio_Drag"); dir.createDirectory();
    const auto out = dir.getChildFile (formatLayerFilename (trackIndex + 1, tracks[trackIndex].name));
    juce::FileOutputStream fos (out);
    if (fos.openedOk()) { fos.setPosition (0); fos.truncate(); buildSingleTrackMidiFile (tracks[trackIndex], trackIndex).writeTo (fos); }
    return out;
}

bool MIDIstudioAudioProcessor::exportSplitLayersZip (const juce::File& targetZip) const
{
    const juce::ScopedLock sl (stateLock);
    const auto tmpDir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("MIDIstudio_ZipExport");
    tmpDir.deleteRecursively(); tmpDir.createDirectory();
    juce::ZipFile::Builder builder;
    auto addMidi = [&] (const juce::String& fname, const juce::MidiFile& mf) {
        const auto f = tmpDir.getChildFile (fname); juce::FileOutputStream fos (f);
        if (fos.openedOk()) { mf.writeTo (fos); fos.flush(); builder.addFile (f, 5, fname); }
    };
    if (includeChords && !chords.empty()) {
        InstrumentTrack ct; ct.name = "Chords Guide";
        for (const auto& c : chords) for (int p : c.midiNotes) ct.notes.push_back ({ 0, p, c.startSec, c.durSec * 0.95, 85 });
        addMidi ("00_Chords_Guide.mid", buildSingleTrackMidiFile (ct, 0));
    }
    int idx = 1;
    for (const auto& trk : tracks) {
        if (trk.isDrum && splitDrums) {
            struct Bucket { const char* name; std::vector<MidiNoteItem> notes; };
            std::vector<Bucket> bs { { "Drums_Kick", {} }, { "Drums_Snare", {} }, { "Drums_HiHat", {} }, { "Drums_Toms", {} }, { "Drums_Cymbals", {} } };
            for (const auto& n : trk.notes) {
                if (n.pitch == 35 || n.pitch == 36) bs[0].notes.push_back (n);
                else if (n.pitch >= 37 && n.pitch <= 40) bs[1].notes.push_back (n);
                else if (n.pitch == 42 || n.pitch == 44 || n.pitch == 46) bs[2].notes.push_back (n);
                else if (n.pitch == 41 || n.pitch == 43 || n.pitch == 45 || n.pitch == 47 || n.pitch == 48 || n.pitch == 50) bs[3].notes.push_back (n);
                else bs[4].notes.push_back (n);
            }
            for (const auto& b : bs) if (!b.notes.empty()) { InstrumentTrack sub = trk; sub.name = b.name; sub.notes = b.notes; addMidi (formatLayerFilename (idx++, b.name), buildSingleTrackMidiFile (sub, idx)); }
        } else { addMidi (formatLayerFilename (idx++, trk.name), buildSingleTrackMidiFile (trk, idx)); }
    }
    const auto rf = tmpDir.getChildFile ("README.txt");
    rf.replaceWithText ("MIDIstudio by solaraudio\nSession: " + sessionName + "\nTempo: " + juce::String (getEffectiveBpm(), 1) + " BPM\nMeter: " + getMeterString() + "\nKey: " + getKeyRootString() + " " + getScaleString() + "\n");
    builder.addFile (rf, 5, "README.txt");
    juce::FileOutputStream zos (targetZip);
    if (!zos.openedOk()) return false;
    zos.setPosition (0); zos.truncate(); return builder.writeToStream (zos, nullptr);
}

void MIDIstudioAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MIDIstudioAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState && xmlState->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessorEditor* MIDIstudioAudioProcessor::createEditor() { return new MIDIstudioAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MIDIstudioAudioProcessor(); }
