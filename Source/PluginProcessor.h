#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>

struct MidiNoteItem { int id = 0; int pitch = 60; double startSec = 0.0; double durSec = 0.5; int velocity = 100; };
struct InstrumentTrack {
    juce::String id, name;
    int program = 0;
    bool isDrum = false;
    juce::Colour colour { 0xffffb84d };
    bool solo = false, mute = false;
    float velScale = 100.0f, velComp = 0.0f;
    std::vector<MidiNoteItem> notes;
};
struct ChordMarker { int bar = 1; double startSec = 0.0, durSec = 2.0; juce::String symbol; std::vector<int> midiNotes; };

class MIDIstudioAudioProcessor : public juce::AudioProcessor
{
public:
    MIDIstudioAudioProcessor();
    ~MIDIstudioAudioProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "MIDIstudio"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::CriticalSection stateLock;
    juce::String sessionName { "Session_01" };
    std::vector<InstrumentTrack> tracks;
    std::vector<ChordMarker> chords;
    juce::StringArray strictInstrumentWhitelist;
    std::atomic<bool> isPlaying { false };
    std::atomic<double> playheadSec { 0.0 }, durationSec { 16.0 }, detectedBpm { 122.5 }, barOffsetSec { 0.0 };
    bool splitDrums = true, includeChords = true, tplTrackNum = true, tplBpm = true, tplKey = false;
    int loopStartBar = 0, loopEndBar = 0;

    void clearSessionToCleanRoll();
    bool loadAudioOrMidiFile (const juce::File& file);
    void detectKeyAndChords();
    void removeGhostNotes();
    void transposeTrack (int trackIndex, int semitones);
    int getProcessedVelocity (const InstrumentTrack& trk, int rawVel) const;
    bool isTrackAudible (const InstrumentTrack& trk) const;
    double getEffectiveBpm() const;
    int getBeatsPerBar() const;
    juce::String getMeterString() const;
    juce::String getKeyRootString() const;
    juce::String getScaleString() const;
    juce::MidiFile buildSingleTrackMidiFile (const InstrumentTrack& trk, int channelIndex) const;
    juce::File writeTempDragMidiForTrack (int trackIndex) const;
    bool exportSplitLayersZip (const juce::File& targetZip) const;

private:
    struct SynthVoice { bool active = false; int pitch = 60; bool isDrum = false; double phase = 0.0, freq = 440.0; float gain = 0.0f; int samplesRemaining = 0, totalSamples = 1; };
    double currentSampleRate = 44100.0, loadedAudioSampleRate = 44100.0;
    juce::AudioBuffer<float> loadedAudioBuffer;
    std::vector<SynthVoice> voices;
    juce::Random rng;
    int nextNoteId = 1;
    void triggerVoice (const InstrumentTrack& trk, const MidiNoteItem& note);
    juce::String formatLayerFilename (int index, const juce::String& label) const;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MIDIstudioAudioProcessor)
};
