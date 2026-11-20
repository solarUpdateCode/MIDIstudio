#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include "PluginProcessor.h"

class MIDIstudioLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MIDIstudioLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool isHighlighted, bool isDown) override;
};

class PianoRollComponent : public juce::Component
{
public:
    explicit PianoRollComponent (MIDIstudioAudioProcessor& p);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    bool drawMode = false;
    bool highlightInKey = true;
    int selectedTrackIdx = 0;
private:
    MIDIstudioAudioProcessor& proc;
    static constexpr int kMinPitch = 33, kMaxPitch = 78;
    static constexpr float kPxPerSec = 78.0f;
    bool isPitchInSelectedKey (int pitch) const;
};

class MIDIstudioAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer,
                                       public juce::KeyListener,
                                       public juce::FileDragAndDropTarget
{
public:
    explicit MIDIstudioAudioProcessorEditor (MIDIstudioAudioProcessor&);
    ~MIDIstudioAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

private:
    void timerCallback() override;
    void syncTempoModeControls();
    void rebuildTrackListUI();

    MIDIstudioAudioProcessor& proc;
    MIDIstudioLookAndFeel lnf;
    juce::TextEditor sessionNameEditor, filterInput, fixedBpmInput;
    juce::TextButton playBtn { "Play" }, pauseBtn { "Pause" }, stopBtn { "Stop" };
    juce::ToggleButton followPlayheadBtn { "Follow Playhead" }, fixedTempoToggle { "FIXED TEMPO" }, inKeyToggle { "In-Key Rows" }, stereoAuditBtn { "Stereo" };
    juce::Label timeLabel;
    juce::TextButton transcribeBtn { "TRANSCRIBE" }, exportLayersBtn { "EXPORT LAYERS" };
    juce::TextButton clearFilterBtn { "Clear" }, transcribeAnotherBtn { "Transcribe Another" };
    juce::TextButton editToolBtn { "Edit" }, drawToolBtn { "Draw" }, ghostBtn { "Ghost Notes" }, octDownBtn { "-1 Oct" }, octUpBtn { "+1 Oct" };
    juce::TextButton halfTempoBtn { "/2" }, doubleTempoBtn { "x2" }, atPlayheadBtn { "AT PLAYHEAD" }, resetTempoBtn { "Back to detected" };
    juce::TextButton splitZipBtn { "SPLIT DAW LAYERS (.ZIP)" };
    juce::Slider mixKnob, quantizeKnob;
    juce::ComboBox meterBox, keyBox, scaleBox;
    PianoRollComponent pianoRoll;
    juce::Viewport trackViewport;
    juce::Component trackContainer;
    juce::OwnedArray<juce::Component> trackRows;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAtt, quantAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> stereoAtt, fixedModeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> meterAtt, keyAtt, scaleAtt;
    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MIDIstudioAudioProcessorEditor)
};
