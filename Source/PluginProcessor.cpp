#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <thread>

namespace {
    const juce::StringArray kMeterChoices { "4/4", "3/4", "6/8", "2/4", "5/4", "7/8" };
    const juce::StringArray kKeyChoices   { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    const juce::StringArray kScaleChoices { "Minor", "Major", "Dorian", "Mixolydian", "Harmonic Minor", "Pentatonic Minor" };
    const juce::Colour kPalette[] = {
        juce::Colour (0xff3de0a0), juce::Colour (0xff4fa8ff), juce::Colour (0xffffb84d),
        juce::Colour (0xfff26d52), juce::Colour (0xffc586ff), juce::Colour (0xffffd98a)
    };
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
    transcribeProgress.store (0.0);
    transcribeProcessedSec.store (0.0);
    transcribeTotalSec.store (0.0);
    transcribeEtaSec.store (-1.0);
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

void MIDIstudioAudioProcessor::appendStreamedNote (const juce::String& instName, int program, bool isDrum, int pitch, double startSec, double durSec, int vel)
{
    const juce::ScopedLock sl (stateLock);
    InstrumentTrack* target = nullptr;
    for (auto& t : tracks) {
        if (t.name.equalsIgnoreCase (instName)) { target = &t; break; }
    }
    if (target == nullptr) {
        InstrumentTrack t;
        t.id = "trk_" + juce::String (tracks.size());
        t.name = instName.toLowerCase();
        t.program = program;
        t.isDrum = isDrum || instName.containsIgnoreCase ("drum");
        t.colour = kPalette[tracks.size() % 6];
        tracks.push_back (std::move (t));
        target = &tracks.back();
    }
    target->notes.push_back ({ nextNoteId++, pitch, startSec, std::max (0.06, durSec), vel });
    durationSec.store (std::max (durationSec.load(), startSec + durSec + 0.5));
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
        const double dur = std::max (4.0, static_cast<double> (nSamp) / reader->sampleRate);
        durationSec.store (dur);
        transcribeTotalSec.store (dur);
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
        tracks.clear();
        chords.clear();
        statusMessage = "Connecting to server...";
    }
    transcribeProgress.store (0.02);
    transcribeProcessedSec.store (0.0);
    transcribeEtaSec.store (-1.0);

    std::thread ([this, audioFile, whitelist, onFinished = std::move (onFinished)]() mutable {
        const double wallStart = juce::Time::getMillisecondCounterHiRes() / 1000.0;
       const auto ext = audioFile.getFileExtension().toLowerCase();
    juce::String mime = "audio/wav";
    if (ext == ".mp3") mime = "audio/mpeg";
    else if (ext == ".flac") mime = "audio/flac";
    else if (ext == ".ogg") mime = "audio/ogg";
else if (ext == ".m4a") mime = "audio/mp4";
       const juce::String localBase = juce::String ("http") + "://127.0.0.1:8222";
const juce::String remoteBase = "https://muscriptor.kyutai.org";

// Auto-launch local MuScriptor server if installed and not yet listening
auto isLocalAlive = [&]() {
    int c = 0;
    auto s = juce::URL (localBase).createInputStream (
        juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs (600).withStatusCode (&c));
    return s != nullptr && c > 0;
};

if (!isLocalAlive()) {
   #if JUCE_WINDOWS
    auto localBat = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory().getChildFile ("start_local_server.bat");
    if (!localBat.existsAsFile()) localBat = juce::File ("C:\\Program Files\\solaraudio\\MIDIstudio\\start_local_server.bat");
    if (localBat.existsAsFile()) localBat.startAsProcess();
   #elif JUCE_MAC
    juce::File localSh ("/Library/Application Support/solaraudio/MIDIstudio/start_local_server.sh");
    if (localSh.existsAsFile()) localSh.startAsProcess();
   #endif
    for (int w = 0; w < 6 && !isLocalAlive(); ++w) juce::Thread::sleep (500);
}

juce::StringArray bases;
if (isLocalAlive()) bases.add (localBase);
bases.add (remoteBase);
        
        bool ok = false;
        juce::String msg = "Could not connect to server";
for (const auto& base : bases) {
    if (ok) break;
    juce::URL sseUrl (base + "/transcribe");
    sseUrl = sseUrl.withFileToUpload ("file", audioFile, mime);
    if (whitelist.isNotEmpty()) sseUrl = sseUrl.withParameter ("instruments", whitelist);

    int statusCode = 0;
    auto options = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                       .withConnectionTimeoutMs (240000)
                       .withExtraHeaders ("Accept: text/event-stream\r\nUser-Agent: Mozilla/5.0 MIDIstudio/1.0\r\n")
                       .withStatusCode (&statusCode);
    
        if (auto stream = sseUrl.createInputStream (options)) {
            if (statusCode == 200) {
                {
                    const juce::ScopedLock sl (stateLock);
                    statusMessage = "Connected - generating MIDI on piano roll...";
                }
                std::map<int, std::pair<size_t, size_t>> indexToNote;
                juce::String pendingLine;
                char buf[256];
juce::String currentEventName;
                while (!stream->isExhausted()) {
                    const int n = stream->read (buf, sizeof (buf));
                    if (n <= 0) break;
                    pendingLine += juce::String::fromUTF8 (buf, n);

                    while (pendingLine.contains ("\n")) {
                        const auto line = pendingLine.upToFirstOccurrenceOf ("\n", false, false).trim();
                        pendingLine = pendingLine.fromFirstOccurrenceOf ("\n", false, false);
                        if (line.isEmpty()) { currentEventName.clear(); continue; }
                        if (line.startsWith ("event:")) { currentEventName = line.fromFirstOccurrenceOf ("event:", false, false).trim(); continue; }
                        if (!line.startsWith ("data:")) continue;
                        const auto jsonText = line.fromFirstOccurrenceOf ("data:", false, false).trim();
                        const auto parsed = juce::JSON::parse (jsonText);
                        if (!parsed.isObject()) continue;

                        const auto evType = parsed.getProperty ("type", currentEventName).toString();
                        if (parsed.hasProperty ("notes")) { if (auto* arr = parsed.getProperty ("notes", juce::var()).getArray()) { for (const auto& nv : *arr) { if (!nv.isObject()) continue; const auto inst = nv.getProperty ("instrument", "piano").toString(); const int prog = static_cast<int> (nv.getProperty ("program", 0)); const bool drum = static_cast<bool> (nv.getProperty ("is_drum", false)) || inst.containsIgnoreCase ("drum"); const int pitch = static_cast<int> (nv.getProperty ("pitch", 60)); const double start = static_cast<double> (nv.getProperty ("start", nv.getProperty ("time", 0.0))); const double dur = static_cast<double> (nv.getProperty ("duration", nv.getProperty ("dur", 0.25))); const int vel = static_cast<int> (nv.getProperty ("velocity", nv.getProperty ("vel", 96))); appendStreamedNote (inst, prog, drum, pitch, start, dur, vel); } } ;
                            }if (evType == "start") { const auto inst = parsed.getProperty ("instrument", "piano").toString(); const int pitch = static_cast<int> (parsed.getProperty ("pitch", 60)); const double st = static_cast<double> (parsed.getProperty ("start_time", parsed.getProperty ("time", 0.0))); const int evIdx = static_cast<int> (parsed.getProperty ("index", -1)); const bool drum = inst.containsIgnoreCase ("drum"); { const juce::ScopedLock sl (stateLock); size_t ti = tracks.size(); for (size_t i = 0; i < tracks.size(); ++i) if (tracks[i].name.equalsIgnoreCase (inst)) { ti = i; break; } if (ti == tracks.size()) { InstrumentTrack tr; tr.id = "trk_" + juce::String (tracks.size()); tr.name = inst.toLowerCase(); tr.program = 0; tr.isDrum = drum; tr.colour = kPalette[tracks.size() % 6]; tracks.push_back (std::move (tr)); } const size_t ni = tracks[ti].notes.size(); tracks[ti].notes.push_back ({ nextNoteId++, pitch, st, 0.15, drum ? 105 : 96 }); if (evIdx >= 0) indexToNote[evIdx] = { ti, ni }; durationSec.store (std::max (durationSec.load(), st + 0.5)); } transcribeProcessedSec.store (std::max (transcribeProcessedSec.load(), st)); } else if (evType == "end") { const double et = static_cast<double> (parsed.getProperty ("end_time", parsed.getProperty ("time", 0.0))); const int startIdx = static_cast<int> (parsed.getProperty ("start_event_index", -1)); auto it = indexToNote.find (startIdx); if (it != indexToNote.end()) { const size_t ti = it->second.first; const size_t ni = it->second.second; { const juce::ScopedLock sl (stateLock); if (ti < tracks.size() && ni < tracks[ti].notes.size()) { tracks[ti].notes[ni].durSec = std::max (0.06, et - tracks[ti].notes[ni].startSec); durationSec.store (std::max (durationSec.load(), et + 0.5)); } } transcribeProcessedSec.store (std::max (transcribeProcessedSec.load(), et)); }
               } else if (evType == "progress") { const double completed = static_cast<double> (parsed.getProperty ("completed", 0.0)); const double total = std::max (1.0, static_cast<double> (parsed.getProperty ("total", 1.0))); const double frac = juce::jlimit (0.02, 0.99, completed / total); transcribeProgress.store (frac); if (transcribeTotalSec.load() > 0.1) transcribeProcessedSec.store (frac * transcribeTotalSec.load()); const double elapsed = (juce::Time::getMillisecondCounterHiRes() / 1000.0) - wallStart; if (frac > 0.03) transcribeEtaSec.store (std::max (0.0, elapsed * (1.0 - frac) / frac)); } else if (evType == "transcription_complete" || parsed.hasProperty ("data") || parsed.hasProperty ("midi")) { const auto b64 = parsed.getProperty ("data", parsed.getProperty ("midi", "")).toString(); if (b64.isNotEmpty()) { juce::MemoryOutputStream mos; if (juce::Base64::convertFromBase64 (mos, b64)) { ok = loadMidiFromMemory (mos.getMemoryBlock(), audioFile.getFileNameWithoutExtension()); } } if (parsed.hasProperty ("beat_grid") && parsed.getProperty ("beat_grid", {}).isObject()) { const auto bg = parsed.getProperty ("beat_grid", {}); detectedBpm.store (static_cast<double> (bg.getProperty ("bpm", detectedBpm.load()))); barOffsetSec.store (static_cast<double> (bg.getProperty ("first_downbeat", barOffsetSec.load()))); const int bpb = static_cast<int> (bg.getProperty ("beats_per_bar", 4)); (void) bpb; } else if (parsed.hasProperty ("bpm")) { detectedBpm.store (static_cast<double> (parsed.getProperty ("bpm", 122.5))); } transcribeProgress.store (1.0); transcribeEtaSec.store (0.0); ok = ok || !tracks.empty(); msg = "Transcription complete"; }
                        if (transcribeTotalSec.load() > 0.1 && transcribeProcessedSec.load() > 0.0) {
                            const double frac = juce::jlimit (0.03, 0.99, transcribeProcessedSec.load() / transcribeTotalSec.load());
                            transcribeProgress.store (frac);
                        }
                    }
                }
                if (!ok && !tracks.empty()) { ok = true; detectKeyAndChords(); msg = "Transcription complete"; }
            }
        }

        // Automatic fallback to blocking /transcribe/midi if SSE stream didn't return MIDI
        if (!ok) {
            {
                const juce::ScopedLock sl (stateLock);
                statusMessage = "Generating full MIDI...";
            }
          juce::URL fbUrl (base + "/transcribe/midi");
fbUrl = fbUrl.withFileToUpload ("file", audioFile, mime);
if (whitelist.isNotEmpty()) fbUrl = fbUrl.withParameter ("instruments", whitelist);
int fbCode = 0;
auto fbOpt = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                 .withConnectionTimeoutMs (240000)
                 .withExtraHeaders ("User-Agent: Mozilla/5.0 MIDIstudio/1.0\r\n")
                 .withStatusCode (&fbCode);
if (auto fbStream = fbUrl.createInputStream (fbOpt)) {
    juce::MemoryBlock resp;
    fbStream->readIntoMemoryBlock (resp);
    if (fbCode == 200 && resp.getSize() > 14) {
        ok = loadMidiFromMemory (resp, audioFile.getFileNameWithoutExtension());
        msg = ok ? "Transcription complete" : "Invalid MIDI returned by server";
    } else {
        msg = "Server HTTP " + juce::String (fbCode) + " - start Local Server from Start Menu";
    }
}
}
}     
        if (ok) transcribeProgress.store (1.0);
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

bool MIDIstudioAudioProcessor::exportFullMidi (const juce::File& targetFile) const
{
    const juce::ScopedLock sl (stateLock);
    juce::MidiFile mf; mf.setTicksPerQuarterNote (480);
    for (size_t i = 0; i < tracks.size(); ++i) {
        if (!isTrackAudible (tracks[i])) continue;
        auto single = buildSingleTrackMidiFile (tracks[i], static_cast<int> (i));
        if (mf.getNumTracks() == 0 && single.getNumTracks() > 0) mf.addTrack (*single.getTrack (0));
        if (single.getNumTracks() > 1) mf.addTrack (*single.getTrack (1));
    }
    juce::FileOutputStream fos (targetFile);
    if (!fos.openedOk()) return false;
    fos.setPosition (0); fos.truncate(); return mf.writeTo (fos);
}

bool MIDIstudioAudioProcessor::exportWavFile (const juce::File& targetWav, bool stereoWithOriginal) const
{
    const juce::ScopedLock sl (stateLock);
    const double sr = 44100.0;
    const int totalSamples = std::max (44100, static_cast<int> (std::min (durationSec.load(), 300.0) * sr));
    juce::AudioBuffer<float> outBuf (2, totalSamples);
    outBuf.clear();
    const double tempoRatio = detectedBpm.load() / std::max (20.0, getEffectiveBpm());

    for (const auto& trk : tracks) {
        if (!isTrackAudible (trk)) continue;
        for (const auto& n : trk.notes) {
            const int s0 = static_cast<int> (n.startSec * tempoRatio * sr);
            const int len = std::min (totalSamples - s0, static_cast<int> (std::max (0.06, n.durSec * tempoRatio) * sr));
            if (s0 < 0 || len <= 0) continue;
            const double freq = trk.isDrum ? (n.pitch <= 36 ? 70.0 : 1200.0) : 440.0 * std::pow (2.0, (n.pitch - 69) / 12.0);
            const float amp = (getProcessedVelocity (trk, n.velocity) / 127.0f) * 0.14f;
            for (int i = 0; i < len; ++i) {
                const float env = std::min (1.0f, i / 120.0f) * (1.0f - static_cast<float> (i) / static_cast<float> (len));
                const float smp = std::sin (static_cast<float> (juce::MathConstants<double>::twoPi * freq * (i / sr))) * amp * env;
                if (stereoWithOriginal) outBuf.addSample (1, s0 + i, smp);
                else { outBuf.addSample (0, s0 + i, smp); outBuf.addSample (1, s0 + i, smp); }
            }
        }
    }

    if (stereoWithOriginal && loadedAudioBuffer.getNumSamples() > 0) {
        const double ratio = loadedAudioSampleRate / sr;
        for (int i = 0; i < totalSamples; ++i) {
            const int idx = static_cast<int> (i * ratio);
            if (idx < loadedAudioBuffer.getNumSamples()) outBuf.setSample (0, i, loadedAudioBuffer.getSample (0, idx) * 0.85f);
        }
    }

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> fos (targetWav.createOutputStream());
    if (!fos) return false;
    fos->setPosition (0); fos->truncate();
    if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (fos.get(), sr, 2, 16, {}, 0))) {
        fos.release();
        return writer->writeFromAudioSampleBuffer (outBuf, 0, totalSamples);
    }
    return false;
}

bool MIDIstudioAudioProcessor::exportSheetMusic (const juce::File& targetFile) const
{
    const auto tmpMid = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("midistudio_sheets.mid");
    if (exportFullMidi (tmpMid)) {
        juce::URL url (juce::String ("https://muscriptor.kyutai.org") + "/sheets");
        url = url.withFileToUpload ("file", tmpMid, "audio/midi");
        int code = 0;
        auto opt = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData).withConnectionTimeoutMs (60000).withStatusCode (&code);
        if (auto stream = url.createInputStream (opt)) {
            if (code == 200) {
                juce::MemoryBlock data;
                stream->readIntoMemoryBlock (data);
                if (data.getSize() > 100) return targetFile.replaceWithData (data.getData(), data.getSize());
            }
        }
    }
    const juce::String xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<score-partwise version=\"4.0\"><work><work-title>" + sessionName + "</work-title></work></score-partwise>\n";
    return targetFile.replaceWithText (xml);
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
