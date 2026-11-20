#include "PluginEditor.h"

MIDIstudioLookAndFeel::MIDIstudioLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff120e0a));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffffd98a));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff574023));
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff120e0a));
    setColour (juce::TextEditor::textColourId, juce::Colour (0xffffd98a));
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff574023));
}

void MIDIstudioLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    const float radius = juce::jmin (w, h) * 0.44f, cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff5dfb4), cx - radius, cy - radius, juce::Colour (0xff6e4f26), cx + radius, cy + radius, false));
    g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (juce::Colour (0xff3c2a13));
    g.drawEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 2.0f);
    juce::Path ptr; ptr.addRoundedRectangle (-1.8f, -radius + 4.0f, 3.6f, radius * 0.48f, 1.5f);
    g.setColour (juce::Colour (0xff23170a));
    g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

void MIDIstudioLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    juce::Colour top = down ? juce::Colour (0xff3a2914) : (highlighted ? juce::Colour (0xff6d4f29) : juce::Colour (0xff523c1f));
    g.setGradientFill (juce::ColourGradient (top, r.getTopLeft(), juce::Colour (0xff1c140a), r.getBottomLeft(), false));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (highlighted ? juce::Colour (0xfff4d38e) : juce::Colour (0xff5a4222));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);
}

PianoRollComponent::PianoRollComponent (MIDIstudioAudioProcessor& p) : proc (p) {}

bool PianoRollComponent::isPitchInSelectedKey (int pitch) const
{
    static const int kScales[6][7] = {
        { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 9, 10 },
        { 0, 2, 4, 5, 7, 9, 10 }, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 3, 5, 7, 10, 0, 0 }
    };
    const int root = static_cast<int> (proc.apvts.getRawParameterValue ("keyRoot")->load());
    const int scale = juce::jlimit (0, 5, static_cast<int> (proc.apvts.getRawParameterValue ("scaleIdx")->load()));
    const int rel = ((pitch % 12) - root + 12) % 12, count = (scale == 5) ? 5 : 7;
    for (int i = 0; i < count; ++i) if (kScales[scale][i] == rel) return true;
    return false;
}

void PianoRollComponent::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds(); g.fillAll (juce::Colour (0xff0f0c09));
    auto chordStrip = bounds.removeFromTop (24);
    g.setColour (juce::Colour (0xff15100b)); g.fillRect (chordStrip);
    g.setColour (juce::Colour (0xff342718)); g.drawHorizontalLine (chordStrip.getBottom() - 1, 0.0f, static_cast<float> (getWidth()));
    auto keysArea = bounds.removeFromLeft (46);
    const int numKeys = kMaxPitch - kMinPitch + 1;
    const float rowH = static_cast<float> (bounds.getHeight()) / static_cast<float> (numKeys);

    for (int p = kMaxPitch; p >= kMinPitch; --p) {
        const float y = bounds.getY() + (kMaxPitch - p) * rowH;
        const int pc = p % 12; const bool black = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
        g.setColour (black ? juce::Colour (0xff1b1611) : juce::Colour (0xffefe3cc));
        g.fillRect (keysArea.getX(), juce::roundToInt (y), black ? 30 : 46, std::max (1, juce::roundToInt (rowH)));
        g.setColour ((highlightInKey && isPitchInSelectedKey (p)) ? (black ? juce::Colour (0x10ffb84d) : juce::Colour (0x1affb84d))
                                                                 : (black ? juce::Colour (0xff0c0907) : juce::Colour (0xff14100b)));
        g.fillRect (bounds.getX(), juce::roundToInt (y), bounds.getWidth(), std::max (1, juce::roundToInt (rowH)));
    }

    const double beatSec = 60.0 / std::max (20.0, proc.getEffectiveBpm());
    const int bpb = proc.getBeatsPerBar();
    for (int b = 0; b * beatSec < 30.0; ++b) {
        float x = bounds.getX() + static_cast<float> ((proc.barOffsetSec.load() + b * beatSec) * kPxPerSec);
        if (x > bounds.getRight()) break;
        g.setColour ((b % bpb == 0) ? juce::Colour (0x40ffc873) : juce::Colour (0x14ffc873));
        g.drawVerticalLine (juce::roundToInt (x), static_cast<float> (bounds.getY()), static_cast<float> (bounds.getBottom()));
    }

    const juce::ScopedLock sl (proc.stateLock);
    for (const auto& c : proc.chords) {
        float cx = bounds.getX() + static_cast<float> (c.startSec * kPxPerSec);
        g.setColour (juce::Colour (0xffffd98a)); g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (c.symbol, juce::roundToInt (cx) + 4, chordStrip.getY(), 60, chordStrip.getHeight(), juce::Justification::centredLeft);
    }

    if (proc.tracks.empty()) {
        g.setColour (juce::Colour (0x66ffd98a)); g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText ("Clean Piano Roll - Drop audio/MIDI or click TRANSCRIBE to begin", bounds, juce::Justification::centred);
    } else {
        const double tempoRatio = proc.detectedBpm.load() / std::max (20.0, proc.getEffectiveBpm());
        for (size_t ti = 0; ti < proc.tracks.size(); ++ti) {
            const auto& trk = proc.tracks[ti]; if (!proc.isTrackAudible (trk)) continue;
            for (const auto& n : trk.notes) {
                if (n.pitch < kMinPitch || n.pitch > kMaxPitch) continue;
                float x = bounds.getX() + static_cast<float> (n.startSec * tempoRatio * kPxPerSec);
                float w = std::max (6.0f, static_cast<float> (n.durSec * tempoRatio * kPxPerSec));
                float y = bounds.getY() + (kMaxPitch - n.pitch) * rowH + 1.0f;
                g.setColour (trk.colour.withAlpha (static_cast<int> (ti) == selectedTrackIdx ? 0.95f : 0.6f));
                g.fillRoundedRectangle (x, y, w, std::max (4.0f, rowH - 2.0f), 2.5f);
            }
        }
    }
    g.setColour (juce::Colour (0xffffd98a));
    g.drawVerticalLine (juce::roundToInt (bounds.getX() + static_cast<float> (proc.playheadSec.load() * kPxPerSec)), static_cast<float> (bounds.getY()), static_cast<float> (bounds.getBottom()));
}

void PianoRollComponent::mouseDown (const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds(); bounds.removeFromTop (24); bounds.removeFromLeft (46);
    if (!bounds.contains (e.getPosition())) return;
    const double sec = std::max (0.0, static_cast<double> (e.x - bounds.getX()) / kPxPerSec);
    if (!drawMode) { proc.playheadSec.store (sec); repaint(); return; }
    const int numKeys = kMaxPitch - kMinPitch + 1;
    const float rowH = static_cast<float> (bounds.getHeight()) / static_cast<float> (numKeys);
    const int pitch = juce::jlimit (kMinPitch, kMaxPitch, kMaxPitch - static_cast<int> ((e.y - bounds.getY()) / rowH));
    const juce::ScopedLock sl (proc.stateLock);
    if (proc.tracks.empty()) { InstrumentTrack t; t.id = "trk_1"; t.name = "midi track 1"; proc.tracks.push_back (t); selectedTrackIdx = 0; }
    auto& trk = proc.tracks[juce::jlimit (0, static_cast<int> (proc.tracks.size()) - 1, selectedTrackIdx)];
    trk.notes.push_back ({ static_cast<int> (trk.notes.size()) + 1, pitch, sec, 0.5, 105 });
    repaint();
}

namespace {
    struct TrackRowComponent : public juce::Component {
        juce::TextButton dragBtn { "::" }, soloBtn { "S" }, muteBtn { "M" };
        juce::Label lbl;
        TrackRowComponent (const juce::String& name, std::function<void()> onDrag, std::function<void()> onSolo, std::function<void()> onMute) {
            dragBtn.setTooltip ("Drag MIDI layer directly to DAW"); dragBtn.onClick = std::move (onDrag);
            lbl.setText (name, juce::dontSendNotification); lbl.setColour (juce::Label::textColourId, juce::Colour (0xfff3e5cc));
            soloBtn.onClick = std::move (onSolo); muteBtn.onClick = std::move (onMute);
            addAndMakeVisible (dragBtn); addAndMakeVisible (lbl); addAndMakeVisible (soloBtn); addAndMakeVisible (muteBtn);
        }
        void resized() override { dragBtn.setBounds (4, 6, 26, 28); lbl.setBounds (34, 6, 132, 28); soloBtn.setBounds (170, 6, 42, 28); muteBtn.setBounds (216, 6, 42, 28); }
    };
}

MIDIstudioAudioProcessorEditor::MIDIstudioAudioProcessorEditor (MIDIstudioAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), pianoRoll (p)
{
    setLookAndFeel (&lnf); setSize (1160, 680); setResizable (true, true); setResizeLimits (920, 560, 1800, 1100);
    addKeyListener (this); setWantsKeyboardFocus (true);

    sessionNameEditor.setText (proc.sessionName, false);
    sessionNameEditor.onTextChange = [this] { proc.sessionName = sessionNameEditor.getText(); };
    addAndMakeVisible (sessionNameEditor);

    playBtn.onClick = [this] { proc.isPlaying.store (true); };
    pauseBtn.onClick = [this] { proc.isPlaying.store (false); };
    stopBtn.onClick = [this] { proc.isPlaying.store (false); proc.playheadSec.store (0.0); };
    followPlayheadBtn.setToggleState (true, juce::dontSendNotification);
    addAndMakeVisible (playBtn); addAndMakeVisible (pauseBtn); addAndMakeVisible (stopBtn); addAndMakeVisible (followPlayheadBtn);

    timeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffd98a));
    timeLabel.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    timeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (timeLabel);

    transcribeBtn.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser> ("Select Audio or MIDI to Transcribe", juce::File{}, "*.wav;*.mp3;*.flac;*.ogg;*.aiff;*.mid;*.midi");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) {
            if (proc.loadAudioOrMidiFile (fc.getResult())) { sessionNameEditor.setText (proc.sessionName, false); rebuildTrackListUI(); pianoRoll.repaint(); }
        });
    };
    exportLayersBtn.onClick = [this] { splitZipBtn.triggerClick(); };
    addAndMakeVisible (transcribeBtn); addAndMakeVisible (exportLayersBtn);

    fixedTempoToggle.onClick = [this] { syncTempoModeControls(); };
    addAndMakeVisible (fixedTempoToggle);

    filterInput.setTextToShowWhenEmpty ("Add an instrument... (empty = detect all)", juce::Colour (0xff8a775c));
    clearFilterBtn.onClick = [this] { filterInput.clear(); proc.strictInstrumentWhitelist.clear(); };
    transcribeAnotherBtn.onClick = [this] { proc.clearSessionToCleanRoll(); rebuildTrackListUI(); pianoRoll.repaint(); };
    addAndMakeVisible (filterInput); addAndMakeVisible (clearFilterBtn); addAndMakeVisible (transcribeAnotherBtn);

    editToolBtn.onClick = [this] { pianoRoll.drawMode = false; };
    drawToolBtn.onClick = [this] { pianoRoll.drawMode = true; };
    ghostBtn.onClick = [this] { proc.removeGhostNotes(); rebuildTrackListUI(); pianoRoll.repaint(); };
    octDownBtn.onClick = [this] { proc.transposeTrack (pianoRoll.selectedTrackIdx, -12); pianoRoll.repaint(); };
    octUpBtn.onClick = [this] { proc.transposeTrack (pianoRoll.selectedTrackIdx, 12); pianoRoll.repaint(); };
    inKeyToggle.setToggleState (true, juce::dontSendNotification);
    inKeyToggle.onClick = [this] { pianoRoll.highlightInKey = inKeyToggle.getToggleState(); pianoRoll.repaint(); };
    addAndMakeVisible (editToolBtn); addAndMakeVisible (drawToolBtn); addAndMakeVisible (ghostBtn);
    addAndMakeVisible (octDownBtn); addAndMakeVisible (octUpBtn); addAndMakeVisible (inKeyToggle); addAndMakeVisible (pianoRoll);

    trackViewport.setViewedComponent (&trackContainer, false); addAndMakeVisible (trackViewport);

    for (auto* k : { &mixKnob, &quantizeKnob }) { k->setSliderStyle (juce::Slider::RotaryVerticalDrag); k->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16); addAndMakeVisible (k); }
    addAndMakeVisible (stereoAuditBtn);

    halfTempoBtn.onClick = [this] { double n = juce::jlimit (30.0, 300.0, proc.getEffectiveBpm() * 0.5); proc.detectedBpm.store (n); if (auto* p = proc.apvts.getParameter ("fixedBpm")) p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (n))); syncTempoModeControls(); };
    doubleTempoBtn.onClick = [this] { double n = juce::jlimit (30.0, 300.0, proc.getEffectiveBpm() * 2.0); proc.detectedBpm.store (n); if (auto* p = proc.apvts.getParameter ("fixedBpm")) p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (n))); syncTempoModeControls(); };
    atPlayheadBtn.onClick = [this] { proc.barOffsetSec.store (proc.playheadSec.load()); pianoRoll.repaint(); };
    resetTempoBtn.onClick = [this] { double d = proc.detectedBpm.load(); if (auto* p = proc.apvts.getParameter ("fixedBpm")) p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (d))); syncTempoModeControls(); };
    fixedBpmInput.onReturnKey = [this] { float v = juce::jlimit (30.0f, 300.0f, fixedBpmInput.getText().getFloatValue()); if (auto* p = proc.apvts.getParameter ("fixedBpm")) p->setValueNotifyingHost (p->convertTo0to1 (v)); pianoRoll.repaint(); };
    addAndMakeVisible (halfTempoBtn); addAndMakeVisible (doubleTempoBtn); addAndMakeVisible (atPlayheadBtn); addAndMakeVisible (fixedBpmInput); addAndMakeVisible (resetTempoBtn);

    meterBox.addItemList ({ "4/4", "3/4", "6/8", "2/4", "5/4", "7/8" }, 1);
    keyBox.addItemList ({ "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }, 1);
    scaleBox.addItemList ({ "Minor", "Major", "Dorian", "Mixolydian", "Harmonic Minor", "Pentatonic Minor" }, 1);
    addAndMakeVisible (meterBox); addAndMakeVisible (keyBox); addAndMakeVisible (scaleBox);

    splitZipBtn.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser> ("Export DAW-Ready Split Layers (.ZIP)", juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (proc.sessionName + "_DAW_Layers.zip"), "*.zip");
        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { proc.exportSplitLayersZip (fc.getResult()); });
    };
    addAndMakeVisible (splitZipBtn);

    mixAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "mix", mixKnob);
    quantAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "quantizeStrength", quantizeKnob);
    stereoAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "stereoSplit", stereoAuditBtn);
    fixedModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "fixedTempo", fixedTempoToggle);
    meterAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "meterIdx", meterBox);
    keyAtt       = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "keyRoot", keyBox);
    scaleAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "scaleIdx", scaleBox);

    syncTempoModeControls(); rebuildTrackListUI(); startTimerHz (30);
}

MIDIstudioAudioProcessorEditor::~MIDIstudioAudioProcessorEditor() { setLookAndFeel (nullptr); }

bool MIDIstudioAudioProcessorEditor::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::spaceKey) { proc.isPlaying.store (!proc.isPlaying.load()); return true; }
    return false;
}

bool MIDIstudioAudioProcessorEditor::isInterestedInFileDrag (const juce::StringArray&) { return true; }

void MIDIstudioAudioProcessorEditor::filesDropped (const juce::StringArray& files, int, int)
{
    if (!files.isEmpty() && proc.loadAudioOrMidiFile (juce::File (files[0]))) { sessionNameEditor.setText (proc.sessionName, false); rebuildTrackListUI(); pianoRoll.repaint(); }
}

void MIDIstudioAudioProcessorEditor::syncTempoModeControls()
{
    const bool isFixed = fixedTempoToggle.getToggleState();
    atPlayheadBtn.setVisible (!isFixed);
    resetTempoBtn.setVisible (isFixed);
    fixedBpmInput.setReadOnly (!isFixed);
    fixedBpmInput.setText (juce::String (proc.getEffectiveBpm(), 1), false);
    resized();
}

void MIDIstudioAudioProcessorEditor::rebuildTrackListUI()
{
    trackRows.clear();
    const juce::ScopedLock sl (proc.stateLock);
    int y = 4;
    for (size_t i = 0; i < proc.tracks.size(); ++i) {
        auto* row = new TrackRowComponent (
            proc.tracks[i].name,
            [this, i] { const auto f = proc.writeTempDragMidiForTrack (static_cast<int> (i)); if (f.existsAsFile()) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this); },
            [this, i] { proc.tracks[i].solo = !proc.tracks[i].solo; pianoRoll.repaint(); },
            [this, i] { proc.tracks[i].mute = !proc.tracks[i].mute; pianoRoll.repaint(); }
        );
        row->setBounds (0, y, 270, 40); trackContainer.addAndMakeVisible (row); trackRows.add (row); y += 44;
    }
    trackContainer.setSize (270, std::max (200, y));
}

void MIDIstudioAudioProcessorEditor::timerCallback()
{
    const double s = proc.playheadSec.load();
    timeLabel.setText (juce::String::formatted ("%02d:%04.1fs", static_cast<int> (s / 60.0), std::fmod (s, 60.0)), juce::dontSendNotification);
    if (proc.isPlaying.load()) pianoRoll.repaint();
}

void MIDIstudioAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe7cca0), 0.0f, 0.0f, juce::Colour (0xffa8834b), 0.0f, static_cast<float> (getHeight()), false));
    g.fillAll();
    g.setColour (juce::Colour (0xff5b3f1a));
    g.setFont (juce::FontOptions ("Georgia", 28.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("MIDIstudio", 18, 10, 180, 34, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("by solaraudio", 18, 40, 145, 16, juce::Justification::centredRight);
}

void MIDIstudioAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (14);
    auto topBar = area.removeFromTop (56); topBar.removeFromLeft (180);
    auto topRight = topBar.removeFromRight (250);
    transcribeBtn.setBounds (topRight.removeFromLeft (118).reduced (2, 10));
    exportLayersBtn.setBounds (topRight.reduced (2, 10));
    sessionNameEditor.setBounds (topBar.removeFromLeft (130).reduced (4, 14));
    playBtn.setBounds (topBar.removeFromLeft (54).reduced (2, 12));
    pauseBtn.setBounds (topBar.removeFromLeft (58).reduced (2, 12));
    stopBtn.setBounds (topBar.removeFromLeft (52).reduced (2, 12));
    followPlayheadBtn.setBounds (topBar.removeFromLeft (120).reduced (4, 14));
    timeLabel.setBounds (topBar.reduced (4, 10));

    auto filterRow = area.removeFromTop (34);
    fixedTempoToggle.setBounds (filterRow.removeFromRight (125).reduced (2));
    transcribeAnotherBtn.setBounds (filterRow.removeFromRight (135).reduced (2));
    clearFilterBtn.setBounds (filterRow.removeFromRight (64).reduced (2));
    filterInput.setBounds (filterRow.reduced (2));

    auto bottom = area.removeFromBottom (86);
    mixKnob.setBounds (bottom.removeFromLeft (90).reduced (4));
    stereoAuditBtn.setBounds (bottom.removeFromLeft (68).reduced (2, 26));
    quantizeKnob.setBounds (bottom.removeFromLeft (90).reduced (4));
    halfTempoBtn.setBounds (bottom.removeFromLeft (46).reduced (3, 22));
    doubleTempoBtn.setBounds (bottom.removeFromLeft (46).reduced (3, 22));
    auto tempoSlot = bottom.removeFromLeft (130).reduced (4);
    if (fixedTempoToggle.getToggleState()) { fixedBpmInput.setBounds (tempoSlot.removeFromTop (34).reduced (2)); resetTempoBtn.setBounds (tempoSlot.reduced (2)); }
    else { atPlayheadBtn.setBounds (tempoSlot.removeFromTop (36).reduced (2)); fixedBpmInput.setBounds (tempoSlot.reduced (2)); }
    splitZipBtn.setBounds (bottom.removeFromRight (195).reduced (4, 14));
    meterBox.setBounds (bottom.removeFromLeft (68).reduced (3, 22));
    keyBox.setBounds (bottom.removeFromLeft (62).reduced (3, 22));
    scaleBox.setBounds (bottom.removeFromLeft (115).reduced (3, 22));

    auto rightMixer = area.removeFromRight (280); trackViewport.setBounds (rightMixer.reduced (4));
    auto prTools = area.removeFromTop (30);
    editToolBtn.setBounds (prTools.removeFromLeft (60).reduced (2));
    drawToolBtn.setBounds (prTools.removeFromLeft (60).reduced (2));
    ghostBtn.setBounds (prTools.removeFromLeft (96).reduced (2));
    octDownBtn.setBounds (prTools.removeFromLeft (56).reduced (2));
    octUpBtn.setBounds (prTools.removeFromLeft (56).reduced (2));
    inKeyToggle.setBounds (prTools.removeFromLeft (105).reduced (2));
    pianoRoll.setBounds (area.reduced (4));
}
