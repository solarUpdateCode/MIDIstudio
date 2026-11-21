#include "PluginEditor.h"

namespace {
    juce::String fmtMinSec (double sec) {
        int s = std::max (0, juce::roundToInt (sec));
        return juce::String::formatted ("%d:%02d", s / 60, s % 60);
    }
}

MIDIstudioLookAndFeel::MIDIstudioLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff120e0a));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffffd98a));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff6b4f29));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffffb84d));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff15110d));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xfff3e5cc));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff27e69b));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff0b0907));
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff120e0a));
    setColour (juce::TextEditor::textColourId, juce::Colour (0xffffd98a)); setColour (juce::TextButton::textColourOffId, juce::Colour (0xff1c1207)); setColour (juce::TextButton::textColourOnId, juce::Colour (0xff120b03));
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff574023));
}

void MIDIstudioLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    const float radius = juce::jmin (w, h) * 0.43f, cx = x + w * 0.5f, cy = y + h * 0.5f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    g.setColour (juce::Colour (0x99080502)); g.fillEllipse (cx - radius + 1.5f, cy - radius + 4.0f, radius * 2.0f, radius * 2.0f); g.setColour (juce::Colour (0xff2c1d0d)); g.fillEllipse (cx - radius, cy - radius, radius * 2.0f, radius * 2.0f); for (int i = 0; i < 36; ++i) { const float a = i * juce::MathConstants<float>::twoPi / 36.0f; const float rIn = radius * 0.78f, rOut = radius * 0.99f; g.setColour (i % 2 == 0 ? juce::Colour (0xff8a6531) : juce::Colour (0xff1a1007)); g.drawLine (cx + std::cos (a) * rIn, cy + std::sin (a) * rIn, cx + std::cos (a) * rOut, cy + std::sin (a) * rOut, 1.4f); } const float innerR = radius * 0.74f, pi = juce::MathConstants<float>::pi; auto drawWedge = [&] (float a0, float a1, juce::Colour col) { juce::Path p; p.addPieSegment (cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, a0, a1, 0.0f); g.setColour (col); g.fillPath (p); }; drawWedge (1.5f * pi, 2.0f * pi, juce::Colour (0xfff5deb0)); drawWedge (0.5f * pi, pi, juce::Colour (0xffe8c682)); drawWedge (0.0f, 0.5f * pi, juce::Colour (0xff7d5b2b)); drawWedge (pi, 1.5f * pi, juce::Colour (0xff7d5b2b)); g.setColour (juce::Colour (0xffc9a25e)); g.drawEllipse (cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 1.2f); juce::Path ptr; ptr.addRoundedRectangle (-1.8f, -innerR + 2.5f, 3.6f, innerR * 0.58f, 1.5f); g.setColour (juce::Colour (0xff231508)); g.fillPath (ptr, juce::AffineTransform::rotation (angle).translated (cx, cy));
}

void MIDIstudioLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool highlighted, bool down)
{
     auto r = b.getLocalBounds().toFloat().reduced (1.0f); if (b.getComponentID() == "darkBtn") { g.setColour (juce::Colour (down ? 0xff140f0a : (highlighted ? 0xff2b2117 : 0xff1e1710))); g.fillRoundedRectangle (r, 6.0f); g.setColour (juce::Colour (highlighted ? 0xff8a6531 : 0xff4f3a1f)); g.drawRoundedRectangle (r, 6.0f, 1.1f); return; } g.setColour (juce::Colour (0x66060402)); g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 6.0f); juce::ColourGradient grad (juce::Colour (0xff9f7d46), r.getX(), r.getY(), juce::Colour (0xff95723c), r.getRight(), r.getY(), false); grad.addColour (0.20, juce::Colour (0xffcfa96c)); grad.addColour (0.45, juce::Colour (0xfff7e4b8)); grad.addColour (0.65, juce::Colour (0xffe5c589)); grad.addColour (0.85, juce::Colour (0xffb89254)); g.setGradientFill (grad); g.fillRoundedRectangle (r, 6.0f); if (down) { g.setColour (juce::Colour (0x26120b03)); g.fillRoundedRectangle (r, 6.0f); } else if (highlighted) { g.setColour (juce::Colour (0x1afffae8)); g.fillRoundedRectangle (r, 6.0f); } g.setColour (juce::Colour (0xe8fffae8)); g.drawLine (r.getX() + 5.0f, r.getY() + 1.0f, r.getRight() - 5.0f, r.getY() + 1.0f, 1.1f); g.setColour (juce::Colour (0x994e3414)); g.drawLine (r.getX() + 5.0f, r.getBottom() - 1.0f, r.getRight() - 5.0f, r.getBottom() - 1.0f, 1.1f); g.setColour (juce::Colour (0xff35230e)); g.drawRoundedRectangle (r, 6.0f, 1.2f);
}
void MIDIstudioLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox& box) { auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (1.0f); const float ax = (float) buttonX + (float) buttonW * 0.5f, ay = (float) buttonY + (float) buttonH * 0.5f; juce::Path arrow; arrow.startNewSubPath (ax - 4.0f, ay - 2.0f); arrow.lineTo (ax, ay + 2.5f); arrow.lineTo (ax + 4.0f, ay - 2.0f); if (box.getComponentID() == "goldCombo") { g.setColour (juce::Colour (0x66060402)); g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 6.0f); juce::ColourGradient grad (juce::Colour (0xff9f7d46), r.getX(), r.getY(), juce::Colour (0xff95723c), r.getRight(), r.getY(), false); grad.addColour (0.20, juce::Colour (0xffcfa96c)); grad.addColour (0.45, juce::Colour (0xfff7e4b8)); grad.addColour (0.65, juce::Colour (0xffe5c589)); grad.addColour (0.85, juce::Colour (0xffb89254)); g.setGradientFill (grad); g.fillRoundedRectangle (r, 6.0f); if (isButtonDown) { g.setColour (juce::Colour (0x26120b03)); g.fillRoundedRectangle (r, 6.0f); } g.setColour (juce::Colour (0xe8fffae8)); g.drawLine (r.getX() + 5.0f, r.getY() + 1.0f, r.getRight() - 5.0f, r.getY() + 1.0f, 1.1f); g.setColour (juce::Colour (0x994e3414)); g.drawLine (r.getX() + 5.0f, r.getBottom() - 1.0f, r.getRight() - 5.0f, r.getBottom() - 1.0f, 1.1f); g.setColour (juce::Colour (0xff35230e)); g.drawRoundedRectangle (r, 6.0f, 1.2f); g.setColour (juce::Colour (0xff1c1207)); g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); } else { g.setColour (juce::Colour (0xff120e0a)); g.fillRoundedRectangle (r, 6.0f); g.setColour (juce::Colour (0xff6b4f29)); g.drawRoundedRectangle (r, 6.0f, 1.1f); g.setColour (juce::Colour (0xffd6a656)); g.strokePath (arrow, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); } }
void MIDIstudioLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat();
   if (b.getComponentID() == "pill") {
    const bool isFixed = b.getToggleState();
    const float cx = r.getCentreX();
    auto sw = juce::Rectangle<float> (cx - 18.0f, r.getCentreY() - 9.0f, 36.0f, 18.0f);
    g.setColour (juce::Colour (0xff2b2013)); g.fillRoundedRectangle (sw, 9.0f);
    g.setColour (juce::Colour (0xff6b4f29)); g.drawRoundedRectangle (sw, 9.0f, 1.2f);
    const float kx = isFixed ? sw.getRight() - 16.0f : sw.getX() + 2.0f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff2d1), kx, sw.getY(), juce::Colour (0xff8a6531), kx + 14.0f, sw.getBottom(), false));
    g.fillEllipse (kx, sw.getY() + 2.0f, 14.0f, 14.0f);
    g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    g.setColour (juce::Colour (isFixed ? 0x99261a0c : 0xff261a0c));
    g.drawText ("FOLLOW TEMPO", r.getX(), r.getY(), sw.getX() - r.getX() - 6.0f, r.getHeight(), juce::Justification::centredRight);
    g.setColour (juce::Colour (isFixed ? 0xff261a0c : 0x99261a0c));
    g.drawText ("FIXED TEMPO", sw.getRight() + 6.0f, r.getY(), r.getRight() - sw.getRight() - 6.0f, r.getHeight(), juce::Justification::centredLeft);
    return;
   }
    auto box = juce::Rectangle<float> (r.getX() + 2.0f, r.getCentreY() - 7.0f, 14.0f, 14.0f);
    g.setColour (b.getToggleState() ? juce::Colour (0xffffb84d) : juce::Colour (0xff1a130c));
    g.fillRoundedRectangle (box, 3.0f);
    g.setColour (juce::Colour (0xff6b4f29)); g.drawRoundedRectangle (box, 3.0f, 1.1f);
    g.setColour (b.getComponentID() == "darkText" ? juce::Colour (0xff261a0c) : juce::Colour (0xfff3e5cc));
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText (b.getButtonText(), box.getRight() + 5.0f, r.getY(), r.getWidth() - 22.0f, r.getHeight(), juce::Justification::centredLeft);
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
    auto bounds = getLocalBounds();
    g.fillAll (juce::Colour (0xff0e0b08));

    // Bottom Live Streaming Progress Bar (matching muscriptor.kyutai.org video)
    auto progBar = bounds.removeFromBottom (26);
    g.setColour (juce::Colour (0xff14100b)); g.fillRect (progBar);
    g.setColour (juce::Colour (0xff352717)); g.drawHorizontalLine (progBar.getY(), 0.0f, static_cast<float> (getWidth()));

    const double prog = proc.transcribeProgress.load();
    auto trackRect = progBar.reduced (10, 10).removeFromLeft (std::max (140, progBar.getWidth() - 290)).toFloat();
    g.setColour (juce::Colour (0xff261d14)); g.fillRoundedRectangle (trackRect, 3.0f);
    if (prog > 0.0) {
        g.setColour (juce::Colour (0xff27e69b));
        g.fillRoundedRectangle (trackRect.withWidth (trackRect.getWidth() * static_cast<float> (juce::jlimit (0.0, 1.0, prog))), 3.0f);
    }
    juce::String progText;
    {
        const juce::ScopedLock sl (proc.stateLock);
        if (proc.isTranscribing.load()) {
            progText = fmtMinSec (proc.transcribeProcessedSec.load()) + " / " + fmtMinSec (proc.transcribeTotalSec.load());
            if (proc.transcribeEtaSec.load() >= 0.0) progText += "   Done in " + fmtMinSec (proc.transcribeEtaSec.load());
            else progText += "   " + proc.statusMessage;
        } else {
            progText = proc.statusMessage;
        }
    }
    g.setColour (juce::Colour (0xffffd98a));
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (progText, static_cast<int> (trackRect.getRight()) + 10, progBar.getY(), progBar.getWidth() - static_cast<int> (trackRect.getRight()) - 16, progBar.getHeight(), juce::Justification::centredRight);

    auto chordStrip = bounds.removeFromTop (24);
    g.setColour (juce::Colour (0xff15100b)); g.fillRect (chordStrip);
    g.setColour (juce::Colour (0xff342718)); g.drawHorizontalLine (chordStrip.getBottom() - 1, 0.0f, static_cast<float> (getWidth()));
    auto keysArea = bounds.removeFromLeft (48);
        const double visibleSec = std::max (2.0, static_cast<double> (bounds.getWidth()) / kPxPerSec); if (followPlayhead && proc.isPlaying.load()) { const double ph = proc.playheadSec.load(); if (ph > scrollOffsetSec + visibleSec * 0.72 || ph < scrollOffsetSec) scrollOffsetSec = std::max (0.0, ph - visibleSec * 0.22); } else if (proc.isTranscribing.load()) { const double gen = proc.transcribeProcessedSec.load(); if (gen > scrollOffsetSec + visibleSec * 0.85) scrollOffsetSec = std::max (0.0, gen - visibleSec * 0.70); }
    const int numKeys = kMaxPitch - kMinPitch + 1;
    const float rowH = static_cast<float> (bounds.getHeight()) / static_cast<float> (numKeys);

    for (int p = kMaxPitch; p >= kMinPitch; --p) {
        const float y = bounds.getY() + (kMaxPitch - p) * rowH;
        const int pc = p % 12; const bool black = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
        g.setColour (black ? juce::Colour (0xff1b1611) : juce::Colour (0xfff1e5ce));
        g.fillRect (keysArea.getX(), juce::roundToInt (y), black ? 31 : 48, std::max (1, juce::roundToInt (rowH)));
        g.setColour ((highlightInKey && isPitchInSelectedKey (p)) ? (black ? juce::Colour (0x12ffb84d) : juce::Colour (0x1dffb84d))
                                                                 : (black ? juce::Colour (0xff0c0907) : juce::Colour (0xff14100b)));
        g.fillRect (bounds.getX(), juce::roundToInt (y), bounds.getWidth(), std::max (1, juce::roundToInt (rowH)));
    }

    const double beatSec = 60.0 / std::max (20.0, proc.getEffectiveBpm());
    const int bpb = proc.getBeatsPerBar();
        g.saveState(); g.reduceClipRegion (bounds);
    for (int b = 0; b * beatSec < 40.0 + scrollOffsetSec + visibleSec; ++b) {
        float x = bounds.getX() + static_cast<float> ((proc.barOffsetSec.load() + b * beatSec - scrollOffsetSec) * kPxPerSec);
        if (x > bounds.getRight()) break;
        g.setColour ((b % bpb == 0) ? juce::Colour (0x44ffc873) : juce::Colour (0x14ffc873));
        g.drawVerticalLine (juce::roundToInt (x), static_cast<float> (bounds.getY()), static_cast<float> (bounds.getBottom()));
    }
    g.restoreState();
    const juce::ScopedLock sl (proc.stateLock);
    for (const auto& c : proc.chords) {
        float cx = bounds.getX() + static_cast<float> ((c.startSec - scrollOffsetSec) * kPxPerSec);
        g.setColour (juce::Colour (0xffffd98a)); g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawText (c.symbol, juce::roundToInt (cx) + 4, chordStrip.getY(), 60, chordStrip.getHeight(), juce::Justification::centredLeft);
    }
    g.saveState(); g.reduceClipRegion (bounds);
    if (proc.tracks.empty()) {
        g.setColour (juce::Colour (0x88ffd98a)); g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText (proc.isTranscribing.load() ? proc.statusMessage : "Clean Piano Roll - Drop audio or click LOAD AUDIO -> START TRANSCRIBE", bounds, juce::Justification::centred);
    } else {
        const double tempoRatio = proc.detectedBpm.load() / std::max (20.0, proc.getEffectiveBpm());
        for (size_t ti = 0; ti < proc.tracks.size(); ++ti) {
            const auto& trk = proc.tracks[ti]; if (!proc.isTrackAudible (trk)) continue;
            for (const auto& n : trk.notes) {
                if (n.pitch < kMinPitch || n.pitch > kMaxPitch) continue;
                float x = bounds.getX() + static_cast<float> ((n.startSec * tempoRatio  - scrollOffsetSec)* kPxPerSec);
                float w = std::max (7.0f, static_cast<float> (n.durSec * tempoRatio * kPxPerSec));
                float y = bounds.getY() + (kMaxPitch - n.pitch) * rowH + 1.0f;
                float nh = std::max (5.0f, rowH - 2.0f);
                g.setColour (trk.colour.withAlpha (0.28f));
                g.fillRoundedRectangle (x - 2.0f, y - 2.0f, w + 4.0f, nh + 4.0f, 4.0f);
                g.setColour (trk.colour.withAlpha (static_cast<int> (ti) == selectedTrackIdx ? 0.96f : 0.72f));
                g.fillRoundedRectangle (x, y, w, nh, 2.5f);
                g.setColour (juce::Colour (0xfffff2cc).withAlpha (0.7f));
                g.drawRoundedRectangle (x, y, w, nh, 2.5f, 0.8f);
            }
        }
    }
        if (proc.isTranscribing.load()) {
            const float genX = bounds.getX() + static_cast<float> ((proc.transcribeProcessedSec.load() - scrollOffsetSec) * kPxPerSec);
            if (genX < bounds.getRight()) { const float vx = std::max (static_cast<float> (bounds.getX()), genX); g.setColour (juce::Colour (0x55000000)); g.fillRect (vx, static_cast<float> (bounds.getY()), static_cast<float> (bounds.getRight()) - vx, static_cast<float> (bounds.getHeight())); }
            if (genX >= bounds.getX() && genX <= bounds.getRight()) { g.setColour (juce::Colour (0x5588ffd9)); g.fillRect (genX - 3.0f, static_cast<float> (bounds.getY()), 6.0f, static_cast<float> (bounds.getHeight())); g.setColour (juce::Colour (0xffffd98a)); g.drawVerticalLine (juce::roundToInt (genX), static_cast<float> (bounds.getY()), static_cast<float> (bounds.getBottom())); }
        }
    const float px = bounds.getX() + static_cast<float> ((proc.playheadSec.load() - scrollOffsetSec) * kPxPerSec);
    g.setColour (juce::Colour (0xffffd98a));
    g.drawVerticalLine (juce::roundToInt (px), static_cast<float> (bounds.getY()), static_cast<float> (bounds.getBottom()));
    juce::Path tri; tri.addTriangle (px - 5.0f, static_cast<float> (bounds.getY()), px + 5.0f, static_cast<float> (bounds.getY()), px, static_cast<float> (bounds.getY()) + 7.0f);
    g.fillPath (tri);
        g.restoreState();
}


void PianoRollComponent::mouseDown (const juce::MouseEvent& e)
{
    auto bounds = getLocalBounds(); bounds.removeFromBottom (26); bounds.removeFromTop (24); bounds.removeFromLeft (48);
    if (!bounds.contains (e.getPosition())) return;
    const double sec = std::max (0.0, scrollOffsetSec + static_cast<double> (e.x - bounds.getX()) / kPxPerSec);
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
void PianoRollComponent::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    const double delta = (std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? -wheel.deltaX : -wheel.deltaY) * 3.5;
    scrollOffsetSec = juce::jlimit (0.0, std::max (0.0, proc.durationSec.load() - 2.0), scrollOffsetSec + delta);
    repaint();
}

namespace {
    struct TrackRowComponent : public juce::Component {
        juce::Colour barCol;
        juce::TextButton dragBtn { "::" }, soloBtn { "SOLO" }, muteBtn { "MUTE" };
        juce::Label lbl;
        TrackRowComponent (const juce::String& name, juce::Colour c, std::function<void()> onDrag, std::function<void()> onSolo, std::function<void()> onMute) : barCol (c) {
            dragBtn.setTooltip ("Drag MIDI layer directly to DAW"); dragBtn.onClick = std::move (onDrag);
            lbl.setText (name, juce::dontSendNotification); lbl.setColour (juce::Label::textColourId, juce::Colour (0xfff3e5cc));
            soloBtn.onClick = std::move (onSolo); muteBtn.onClick = std::move (onMute);
            addAndMakeVisible (dragBtn); addAndMakeVisible (lbl); addAndMakeVisible (soloBtn); dragBtn.setComponentID ("darkBtn"); dragBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffebd6b3)); soloBtn.setComponentID ("darkBtn"); soloBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffebd6b3)); muteBtn.setComponentID ("darkBtn"); muteBtn.setColour (juce::TextButton::textColourOffId, juce::Colour (0xffebd6b3)); addAndMakeVisible (muteBtn);
        }
        void paint (juce::Graphics& g) override {
            g.setColour (juce::Colour (0xff16110c)); g.fillRect (getLocalBounds());
            g.setColour (barCol); g.fillRoundedRectangle (28.0f, 8.0f, 4.0f, 26.0f, 2.0f);
            g.setColour (juce::Colour (0xff69e07f)); g.fillEllipse (160.0f, 10.0f, 4.5f, 4.5f);
            g.setColour (juce::Colour (0xffffbe4f)); g.fillEllipse (160.0f, 18.0f, 4.5f, 4.5f);
            g.setColour (juce::Colour (0xfff25f4c)); g.fillEllipse (160.0f, 26.0f, 4.5f, 4.5f);
            g.setColour (juce::Colour (0xff2b2015)); g.drawHorizontalLine (getHeight() - 1, 0.0f, static_cast<float> (getWidth()));
        }
        void resized() override { dragBtn.setBounds (4, 8, 20, 26); lbl.setBounds (36, 7, 120, 28); soloBtn.setBounds (170, 8, 46, 26); muteBtn.setBounds (220, 8, 46, 26); }
    };
}

MIDIstudioAudioProcessorEditor::MIDIstudioAudioProcessorEditor (MIDIstudioAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p), pianoRoll (p)
{
    setLookAndFeel (&lnf); setSize (1220, 720); setResizable (true, true); setResizeLimits (980, 600, 1800, 1120);
    addKeyListener (this); setWantsKeyboardFocus (true);

    sessionNameEditor.setText (proc.sessionName, false);
    sessionNameEditor.onTextChange = [this] { proc.sessionName = sessionNameEditor.getText(); };
    addAndMakeVisible (sessionNameEditor);

    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffcfa96e));
    statusLabel.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    addAndMakeVisible (statusLabel);

    playBtn.onClick = [this] { proc.isPlaying.store (true); };
    pauseBtn.onClick = [this] { proc.isPlaying.store (false); };
    stopBtn.onClick = [this] { proc.isPlaying.store (false); proc.playheadSec.store (0.0); pianoRoll.scrollOffsetSec = 0.0; };
    followPlayheadBtn.setToggleState (true, juce::dontSendNotification);
        followPlayheadBtn.onClick = [this] { pianoRoll.followPlayhead = followPlayheadBtn.getToggleState(); };
    addAndMakeVisible (playBtn); addAndMakeVisible (pauseBtn); addAndMakeVisible (stopBtn); addAndMakeVisible (followPlayheadBtn);

    timeLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffd98a));
    timeLabel.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    timeLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (timeLabel);

    startTranscribeBtn.setComponentID ("gold");
    splitZipBtn.setComponentID ("gold");
    fixedTempoToggle.setComponentID ("pill");
    stereoAuditBtn.setComponentID ("darkText");

    loadAudioBtn.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser> ("Select Audio or MIDI File", juce::File{}, "*.wav;*.mp3;*.flac;*.ogg;*.m4a;*.aiff;*.mid;*.midi");
        fileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) {
            if (proc.loadAudioOrMidiFile (fc.getResult())) { sessionNameEditor.setText (proc.sessionName, false); rebuildTrackListUI(); pianoRoll.repaint(); }
        });
    };
    startTranscribeBtn.onClick = [this] {
        if (!proc.loadedAudioFile.existsAsFile()) { loadAudioBtn.triggerClick(); return; }
        startTranscribeBtn.setEnabled (false);
        startTranscribeBtn.setButtonText ("TRANSCRIBING...");
        proc.startMuscriptorTranscription ([this] (bool, juce::String) {
            startTranscribeBtn.setEnabled (true);
            startTranscribeBtn.setButtonText ("START TRANSCRIBE");
            rebuildTrackListUI();
            syncTempoModeControls();
            pianoRoll.repaint();
        });
    };
    exportLayersBtn.onClick = [this] { splitZipBtn.triggerClick(); };
    addAndMakeVisible (loadAudioBtn); addAndMakeVisible (startTranscribeBtn); addAndMakeVisible (exportLayersBtn);

    fixedTempoToggle.onClick = [this] { syncTempoModeControls(); };
    addAndMakeVisible (fixedTempoToggle);

    instrumentDropdown.setTextWhenNothingSelected ("Add an instrument... (all 35 MuScriptor instruments)");
    instrumentDropdown.addItemList (MIDIstudioAudioProcessor::getMuscriptorInstruments(), 1);
    instrumentDropdown.onChange = [this] {
        const auto chosen = instrumentDropdown.getText();
        if (chosen.isNotEmpty() && !proc.strictInstrumentWhitelist.contains (chosen)) proc.strictInstrumentWhitelist.add (chosen);
        instrumentDropdown.setSelectedId (0, juce::dontSendNotification);
        updateInstrumentWhitelistDisplay();
    };
    clearFilterBtn.onClick = [this] { proc.strictInstrumentWhitelist.clear(); updateInstrumentWhitelistDisplay(); };
    transcribeAnotherBtn.onClick = [this] { proc.clearSessionToCleanRoll(); rebuildTrackListUI(); pianoRoll.repaint(); };
    whitelistLabel.setColour (juce::Label::textColourId, juce::Colour (0xffffd98a));
    whitelistLabel.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    addAndMakeVisible (instrumentDropdown); addAndMakeVisible (whitelistLabel); addAndMakeVisible (clearFilterBtn); addAndMakeVisible (transcribeAnotherBtn);

    editToolBtn.onClick = [this] { pianoRoll.drawMode = false; };
    drawToolBtn.onClick = [this] { pianoRoll.drawMode = true; };
    ghostBtn.onClick = [this] { proc.removeGhostNotes(); rebuildTrackListUI(); pianoRoll.repaint(); };
    octDownBtn.onClick = [this] { proc.transposeTrack (pianoRoll.selectedTrackIdx, -12); pianoRoll.repaint(); };
    octUpBtn.onClick = [this] { proc.transposeTrack (pianoRoll.selectedTrackIdx, 12); pianoRoll.repaint(); };
    inKeyToggle.setToggleState (true, juce::dontSendNotification);
    inKeyToggle.onClick = [this] { pianoRoll.highlightInKey = inKeyToggle.getToggleState(); pianoRoll.repaint(); };
    addAndMakeVisible (editToolBtn); addAndMakeVisible (drawToolBtn); addAndMakeVisible (ghostBtn);
    addAndMakeVisible (octDownBtn); addAndMakeVisible (octUpBtn); addAndMakeVisible (inKeyToggle); addAndMakeVisible (pianoRoll);

    trackViewport.setViewedComponent (&trackContainer, false); addAndMakeVisible (trackViewport); for (auto* tb : { &editToolBtn, &drawToolBtn, &ghostBtn, &octDownBtn, &octUpBtn }) { tb->setComponentID ("darkBtn"); tb->setColour (juce::TextButton::textColourOffId, juce::Colour (0xffebd6b3)); }

    for (auto* k : { &mixKnob, &quantizeKnob }) { k->setSliderStyle (juce::Slider::RotaryVerticalDrag); k->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 16); addAndMakeVisible (k); }
    for (auto* l : { &mixLabel, &quantLabel, &bpmCaption }) { l->setColour (juce::Label::textColourId, juce::Colour (0xff261a0c)); l->setFont (juce::FontOptions (11.0f, juce::Font::bold)); l->setJustificationType (juce::Justification::centred); addAndMakeVisible (l); }
    mixLabel.setText ("Original <-> MIDI Mix", juce::dontSendNotification);
    quantLabel.setText ("Quantize Strength", juce::dontSendNotification);
    bpmCaption.setText ("Tempo BPM", juce::dontSendNotification);
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
    for (auto* cb : { &meterBox, &keyBox, &scaleBox }) { cb->setComponentID ("goldCombo"); cb->setColour (juce::ComboBox::textColourId, juce::Colour (0xff1c1207)); }

    downloadMenuBtn.onClick = [this] { showDownloadPopupMenu(); };
    splitZipBtn.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser> ("Export DAW-Ready Split Layers (.ZIP)", juce::File::getSpecialLocation (juce::File::userDesktopDirectory).getChildFile (proc.sessionName + "_DAW_Layers.zip"), "*.zip");
        fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { proc.exportSplitLayersZip (fc.getResult()); });
    };
    addAndMakeVisible (downloadMenuBtn); addAndMakeVisible (splitZipBtn);

    mixAtt       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "mix", mixKnob);
    quantAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "quantizeStrength", quantizeKnob);
    stereoAtt    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "stereoSplit", stereoAuditBtn);
    fixedModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "fixedTempo", fixedTempoToggle);
    meterAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "meterIdx", meterBox);
    keyAtt       = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "keyRoot", keyBox);
    scaleAtt     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "scaleIdx", scaleBox);

    updateInstrumentWhitelistDisplay();
    syncTempoModeControls();
    rebuildTrackListUI();
    startTimerHz (30);
}

MIDIstudioAudioProcessorEditor::~MIDIstudioAudioProcessorEditor() { setLookAndFeel (nullptr); }

void MIDIstudioAudioProcessorEditor::showDownloadPopupMenu()
{
    juce::PopupMenu menu;
    menu.addItem (1, "MIDI file...");
    menu.addItem (2, "WAV - transcription only");
    menu.addItem (3, "WAV - stereo with original");
    menu.addItem (4, "Sheet music");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&downloadMenuBtn), [this] (int choice) {
        const auto desktop = juce::File::getSpecialLocation (juce::File::userDesktopDirectory);
        if (choice == 1) {
            fileChooser = std::make_unique<juce::FileChooser> ("Save MIDI File", desktop.getChildFile (proc.sessionName + ".mid"), "*.mid");
            fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { proc.exportFullMidi (fc.getResult()); });
        } else if (choice == 2) {
            fileChooser = std::make_unique<juce::FileChooser> ("Save WAV (Transcription Only)", desktop.getChildFile (proc.sessionName + "_midi.wav"), "*.wav");
            fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { proc.exportWavFile (fc.getResult(), false); });
        } else if (choice == 3) {
            fileChooser = std::make_unique<juce::FileChooser> ("Save WAV (Stereo with Original)", desktop.getChildFile (proc.sessionName + "_stereo_audit.wav"), "*.wav");
            fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { proc.exportWavFile (fc.getResult(), true); });
        } else if (choice == 4) {
            fileChooser = std::make_unique<juce::FileChooser> ("Save Sheet Music", desktop.getChildFile (proc.sessionName + "_sheets.zip"), "*.zip;*.musicxml");
            fileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc) { proc.exportSheetMusic (fc.getResult()); });
        }
    });
}

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

void MIDIstudioAudioProcessorEditor::updateInstrumentWhitelistDisplay()
{
    whitelistLabel.setText (proc.strictInstrumentWhitelist.isEmpty()
        ? "Detecting all 35 instruments"
        : "Whitelist: " + proc.strictInstrumentWhitelist.joinIntoString (", "), juce::dontSendNotification);
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
    lastTrackCount = proc.tracks.size();
    int y = 4;
    for (size_t i = 0; i < proc.tracks.size(); ++i) {
        auto* row = new TrackRowComponent (
            proc.tracks[i].name, proc.tracks[i].colour,
            [this, i] { const auto f = proc.writeTempDragMidiForTrack (static_cast<int> (i)); if (f.existsAsFile()) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this); },
            [this, i] { proc.tracks[i].solo = !proc.tracks[i].solo; pianoRoll.repaint(); },
            [this, i] { proc.tracks[i].mute = !proc.tracks[i].mute; pianoRoll.repaint(); }
        );
        row->setBounds (0, y, 272, 42); trackContainer.addAndMakeVisible (row); trackRows.add (row); y += 44;
    }
    trackContainer.setSize (272, std::max (220, y));
}

void MIDIstudioAudioProcessorEditor::timerCallback()
{
    const double s = proc.playheadSec.load();
    timeLabel.setText (juce::String::formatted ("%02d:%04.1fs", static_cast<int> (s / 60.0), std::fmod (s, 60.0)), juce::dontSendNotification);
    size_t currentTracks = 0;
    {
        const juce::ScopedLock sl (proc.stateLock);
        statusLabel.setText (proc.statusMessage, juce::dontSendNotification);
        currentTracks = proc.tracks.size();
    }
    if (currentTracks != lastTrackCount) rebuildTrackListUI();
    if (proc.isPlaying.load() || proc.isTranscribing.load()) pianoRoll.repaint();
}

void MIDIstudioAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto outer = getLocalBounds().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff6e2ba), 0.0f, 0.0f, juce::Colour (0xff6e4f26), outer.getWidth(), outer.getHeight(), false));
    g.fillRoundedRectangle (outer, 16.0f);
    auto face = outer.reduced (4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8cda2), 0.0f, face.getY(), juce::Colour (0xffa8834b), 0.0f, face.getBottom(), false));
    g.fillRoundedRectangle (face, 13.0f);
    g.setColour (juce::Colour (0x08281808));
    for (int y = 6; y < getHeight() - 6; y += 3) g.drawHorizontalLine (y, 6.0f, static_cast<float> (getWidth() - 6));

    g.setColour (juce::Colour (0xff5b3f1a));
    g.setFont (juce::FontOptions ("Georgia", 30.0f, juce::Font::bold | juce::Font::italic));
    g.drawText ("MIDIstudio", 20, 10, 178, 36, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText ("by solaraudio", 20, 42, 148, 16, juce::Justification::centredRight);

    auto oled = juce::Rectangle<float> (200.0f, 12.0f, static_cast<float> (getWidth()) - 530.0f, 52.0f);
    g.setColour (juce::Colour (0xff120e0a)); g.fillRoundedRectangle (oled, 9.0f);
    g.setColour (juce::Colour (0xff4e381d)); g.drawRoundedRectangle (oled, 9.0f, 1.5f);

    auto strip = juce::Rectangle<float> (16.0f, 72.0f, static_cast<float> (getWidth()) - 32.0f, 36.0f);
    g.setColour (juce::Colour (0xe618110a)); g.fillRoundedRectangle (strip, 8.0f);
    g.setColour (juce::Colour (0xff543d20)); g.drawRoundedRectangle (strip, 8.0f, 1.2f);
auto bottomOled = meterBox.getBounds().getUnion (scaleBox.getBounds()).toFloat().expanded (6.0f, 5.0f);
g.setColour (juce::Colour (0xff120e0a)); g.fillRoundedRectangle (bottomOled, 8.0f);
g.setColour (juce::Colour (0xff4e381d)); g.drawRoundedRectangle (bottomOled, 8.0f, 1.5f);
}

void MIDIstudioAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto topBar = area.removeFromTop (52);
    topBar.removeFromLeft (188);
    auto topRight = topBar.removeFromRight (312);
auto topBtns = topRight.removeFromTop (30);
loadAudioBtn.setBounds (topBtns.removeFromLeft (90).reduced (2, 2));
startTranscribeBtn.setBounds (topBtns.removeFromLeft (124).reduced (2, 2));
exportLayersBtn.setBounds (topBtns.reduced (2, 2));
fixedTempoToggle.setBounds (topRight.withSizeKeepingCentre (250, 22));

    auto oledLeft = topBar.removeFromLeft (145);
    sessionNameEditor.setBounds (oledLeft.removeFromTop (26).reduced (4, 2));
    statusLabel.setBounds (oledLeft.reduced (4, 0));
    playBtn.setBounds (topBar.removeFromLeft (46).reduced (2, 10));
    pauseBtn.setBounds (topBar.removeFromLeft (52).reduced (2, 10));
    stopBtn.setBounds (topBar.removeFromLeft (46).reduced (2, 10));
    followPlayheadBtn.setBounds (topBar.removeFromLeft (118).reduced (2, 12));
    timeLabel.setBounds (topBar.reduced (6, 8));

    area.removeFromTop (6);
    auto filterRow = area.removeFromTop (36).reduced (6, 4);

    transcribeAnotherBtn.setBounds (filterRow.removeFromRight (132).reduced (2, 0));
    clearFilterBtn.setBounds (filterRow.removeFromRight (58).reduced (2, 0));
    instrumentDropdown.setBounds (filterRow.removeFromLeft (280).reduced (2, 0));
    whitelistLabel.setBounds (filterRow.reduced (6, 0));

    area.removeFromTop (6);
    auto bottom = area.removeFromBottom (94);
    auto mixCol = bottom.removeFromLeft (118);
    mixLabel.setBounds (mixCol.removeFromTop (16));
    mixKnob.setBounds (mixCol.removeFromTop (56).reduced (12, 0));
    stereoAuditBtn.setBounds (mixCol.reduced (16, 0));

    auto quantCol = bottom.removeFromLeft (108);
    quantLabel.setBounds (quantCol.removeFromTop (16));
    quantizeKnob.setBounds (quantCol.removeFromTop (56).reduced (10, 0));

    halfTempoBtn.setBounds (bottom.removeFromLeft (44).reduced (3, 28));
    doubleTempoBtn.setBounds (bottom.removeFromLeft (44).reduced (3, 28));

    auto tempoSlot = bottom.removeFromLeft (130).reduced (4, 8);
    bpmCaption.setBounds (tempoSlot.removeFromTop (14));
    if (fixedTempoToggle.getToggleState()) { fixedBpmInput.setBounds (tempoSlot.removeFromTop (32).reduced (2)); resetTempoBtn.setBounds (tempoSlot.reduced (2)); }
    else { atPlayheadBtn.setBounds (tempoSlot.removeFromTop (34).reduced (2)); fixedBpmInput.setBounds (tempoSlot.reduced (2)); }

    auto dlCol = bottom.removeFromRight (210).reduced (4, 6);
    downloadMenuBtn.setBounds (dlCol.removeFromTop (28).reduced (2, 1));
    splitZipBtn.setBounds (dlCol.reduced (2, 3));

    meterBox.setBounds (bottom.removeFromLeft (66).reduced (3, 26));
    keyBox.setBounds (bottom.removeFromLeft (60).reduced (3, 26));
    scaleBox.setBounds (bottom.removeFromLeft (115).reduced (3, 26));

    auto rightMixer = area.removeFromRight (282);
    trackViewport.setBounds (rightMixer.reduced (3));
    auto prTools = area.removeFromTop (30);
    editToolBtn.setBounds (prTools.removeFromLeft (56).reduced (2));
    drawToolBtn.setBounds (prTools.removeFromLeft (56).reduced (2));
    ghostBtn.setBounds (prTools.removeFromLeft (92).reduced (2));
    octDownBtn.setBounds (prTools.removeFromLeft (56).reduced (2));
    octUpBtn.setBounds (prTools.removeFromLeft (56).reduced (2));
    inKeyToggle.setBounds (prTools.removeFromLeft (105).reduced (2));
    pianoRoll.setBounds (area.reduced (3));
}
