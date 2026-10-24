#include "PluginEditor.h"
#include <BinaryData.h>

//==============================================================================
juce::Font NinetyLook::font (float h, bool bold)
{
    return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
}

NinetyLook::NinetyLook()
{
    setColour (juce::Slider::textBoxTextColourId,       juce::Colours::white.withAlpha (0.85f));
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId,               juce::Colours::white.withAlpha (0.8f));
    setColour (juce::ComboBox::backgroundColourId,      juce::Colour (0xff231a1a));
    setColour (juce::ComboBox::outlineColourId,         red.withAlpha (0.6f));
    setColour (juce::ComboBox::textColourId,            juce::Colours::white);
    setColour (juce::ComboBox::arrowColourId,           red);
    setColour (juce::PopupMenu::backgroundColourId,     juce::Colour (0xff1c1414));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, red);
    setColour (juce::PopupMenu::textColourId,           juce::Colours::white);
}

juce::Font NinetyLook::getComboBoxFont (juce::ComboBox&) { return font (13.f); }
juce::Font NinetyLook::getLabelFont (juce::Label&)       { return font (12.f, true); }

void NinetyLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                    float startAngle, float endAngle, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float r  = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto  c  = b.getCentre();
    const float a  = startAngle + pos * (endAngle - startAngle);

    // body
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a2d2d), c.x, c.y - r,
                                             juce::Colour (0xff0c0808), c.x, c.y + r, false));
    g.fillEllipse (c.x - r * 0.74f, c.y - r * 0.74f, r * 1.48f, r * 1.48f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (c.x - r * 0.74f, c.y - r * 0.74f, r * 1.48f, r * 1.48f, 1.5f);

    // track
    juce::Path track;
    track.addCentredArc (c.x, c.y, r - 2.f, r - 2.f, 0.f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2b2020));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc + glow
    juce::Path val;
    val.addCentredArc (c.x, c.y, r - 2.f, r - 2.f, 0.f, startAngle, a, true);
    g.setColour (red.withAlpha (0.25f));
    g.strokePath (val, juce::PathStrokeType (8.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (red);
    g.strokePath (val, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // pointer
    const float s = std::sin (a), co = std::cos (a);
    g.setColour (juce::Colours::white);
    g.drawLine (c.x + s * r * 0.30f, c.y - co * r * 0.30f,
                c.x + s * r * 0.66f, c.y - co * r * 0.66f, 2.5f);
}

void NinetyLook::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.f);
    const bool on = b.getToggleState();
    g.setColour (on ? red.withAlpha (hl ? 1.f : 0.9f) : juce::Colour (0xff231a1a).brighter (hl ? 0.15f : 0.f));
    g.fillRoundedRectangle (r, 5.f);
    g.setColour (on ? red.brighter (0.4f) : red.withAlpha (0.5f));
    g.drawRoundedRectangle (r, 5.f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (on ? 1.f : 0.65f));
    g.setFont (font (12.f, true));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

//==============================================================================
NinetyBassEditor::Knob::Knob (juce::AudioProcessorValueTreeState& a, const char* id, const juce::String& name)
{
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (label);

    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 84, 16);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (slider);

    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (a, id, slider);
    if (auto* p = a.getParameter (id))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
}

void NinetyBassEditor::Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

//==============================================================================
NinetyBassEditor::Knob* NinetyBassEditor::addKnob (const char* id, const juce::String& name)
{
    knobs.push_back (std::make_unique<Knob> (proc.apvts, id, name));
    auto* kp = knobs.back().get();
    addAndMakeVisible (kp);
    k[id] = kp;
    return kp;
}

void NinetyBassEditor::addToggle (juce::ToggleButton& b, const char* id, const juce::String& text)
{
    b.setButtonText (text);
    addAndMakeVisible (b);
    buttonAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, b));
}

void NinetyBassEditor::addCombo (juce::ComboBox& c, const char* id, const juce::StringArray& items)
{
    c.addItemList (items, 1);
    addAndMakeVisible (c);
    comboAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, c));
}

NinetyBassEditor::NinetyBassEditor (NinetyBassProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    // Knobs
    addKnob (ID::inGain, "INPUT");        addKnob (ID::xover,    "CROSSOVER");
    addKnob (ID::boostDb,"SUB BOOST");    addKnob (ID::boostHz,  "BOOST FREQ");
    addKnob (ID::boostQ, "BOOST Q");      addKnob (ID::tight,    "TIGHT");
    addKnob (ID::harm,   "HARMONICS");
    addKnob (ID::lowDrive,"CRUNCH");      addKnob (ID::lowBias,  "BIAS");
    addKnob (ID::lowCeil,"LOW CLIP");     addKnob (ID::lowLevel, "LOW LEVEL");
    addKnob (ID::punch,  "PUNCH");
    addKnob (ID::clickDb,"CLICK");        addKnob (ID::highDrive,"GRIT");
    addKnob (ID::highLevel,"HIGH LEVEL");
    addKnob (ID::tone,   "TONE");         addKnob (ID::mix,      "MIX");
    addKnob (ID::outGain,"OUTPUT");       addKnob (ID::ceiling,  "CEILING");

    addToggle (bypassBtn,   ID::bypass,   "BYPASS");
    addToggle (polarityBtn, ID::polarity, "POLARITY  Ø");
    addToggle (monoBtn,     ID::monoSub,  "MONO SUB");
    addToggle (limiterBtn,  ID::limiter,  "LIMITER");

    addCombo (lowTypeBox,  ID::lowType,  { "Warm", "Tube", "Tape", "Hard Clip", "Fold", "Diode" });
    addCombo (highTypeBox, ID::highType, { "Warm", "Tube", "Tape", "Hard Clip", "Fold", "Diode" });
    addCombo (osBox,       ID::os,       { "1x", "2x", "4x" });

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    setSize (1000, 590);
    startTimerHz (30);
}

NinetyBassEditor::~NinetyBassEditor() { setLookAndFeel (nullptr); }

//==============================================================================
void NinetyBassEditor::resized()
{
    pHeader = { 0, 0, getWidth(), 96 };
    pInput  = { 16,  108, 200, 262 };
    pLow    = { 228, 108, 520, 262 };
    pHigh   = { 760, 108, 224, 262 };
    pOut    = { 16,  382, 568, 160 };
    pMeter  = { 596, 382, 388, 160 };

    auto place = [this] (const char* id, int x, int y) { k[id]->setBounds (x, y, 92, 104); };

    // Input / Crossover
    place (ID::inGain, pInput.getX() + 6,   pInput.getY() + 34);
    place (ID::xover,  pInput.getX() + 102, pInput.getY() + 34);
    polarityBtn.setBounds (pInput.getX() + 16, pInput.getY() + 156, 168, 30);
    monoBtn.setBounds     (pInput.getX() + 16, pInput.getY() + 196, 168, 30);

    // Low
    const int lx = pLow.getX() + 6;
    const int r1 = pLow.getY() + 34, r2 = pLow.getY() + 150;
    const char* row1[] { ID::boostDb, ID::boostHz, ID::boostQ, ID::tight, ID::harm };
    const char* row2[] { ID::lowDrive, ID::lowBias, ID::lowCeil, ID::lowLevel, ID::punch };
    for (int i = 0; i < 5; ++i) { place (row1[i], lx + i * 101, r1); place (row2[i], lx + i * 101, r2); }
    lowTypeBox.setBounds (pLow.getRight() - 150, pLow.getY() + 6, 140, 24);

    // High
    place (ID::clickDb,   pHigh.getX() + 8,   pHigh.getY() + 34);
    place (ID::highDrive, pHigh.getX() + 112, pHigh.getY() + 34);
    place (ID::highLevel, pHigh.getX() + 60,  pHigh.getY() + 150);
    highTypeBox.setBounds (pHigh.getRight() - 120, pHigh.getY() + 6, 110, 24);

    // Output
    place (ID::tone,    pOut.getX() + 8,   pOut.getY() + 34);
    place (ID::mix,     pOut.getX() + 108, pOut.getY() + 34);
    place (ID::outGain, pOut.getX() + 208, pOut.getY() + 34);
    place (ID::ceiling, pOut.getX() + 308, pOut.getY() + 34);
    limiterBtn.setBounds (pOut.getX() + 428, pOut.getY() + 56, 120, 32);

    // Header
    presetBox.setBounds (610, 42, 210, 28);
    osBox.setBounds     (832, 42, 62, 28);
    bypassBtn.setBounds (906, 42, 78, 28);
}

//==============================================================================
void NinetyBassEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    auto rf = r.toFloat();
    g.setColour (NinetyLook::panel);
    g.fillRoundedRectangle (rf, 10.f);
    g.setColour (NinetyLook::red.withAlpha (0.55f));
    g.drawRoundedRectangle (rf.reduced (0.5f), 10.f, 1.2f);
    g.setColour (NinetyLook::red);
    g.setFont (NinetyLook::font (13.f, true));
    g.drawText (title, r.getX() + 14, r.getY() + 6, r.getWidth() - 28, 22, juce::Justification::centredLeft);
}

void NinetyBassEditor::drawMeter (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& name, float lin)
{
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    g.setFont (NinetyLook::font (12.f, true));
    g.drawText (name, r.removeFromLeft (44), juce::Justification::centredLeft);

    g.setColour (juce::Colour (0xff0a0707));
    g.fillRoundedRectangle (r, 4.f);

    const float db   = juce::Decibels::gainToDecibels (lin, -60.f);
    const float frac = juce::jlimit (0.f, 1.f, (db + 60.f) / 66.f);
    auto bar = r.reduced (2.f);
    bar.setWidth (bar.getWidth() * frac);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1fa84a), r.getX(), 0.f,
                                             NinetyLook::red, r.getRight(), 0.f, false));
    g.fillRoundedRectangle (bar, 3.f);

    // 0 dB marker
    const float x0 = r.getX() + 2.f + (r.getWidth() - 4.f) * (60.f / 66.f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawLine (x0, r.getY(), x0, r.getBottom(), 1.f);
}

void NinetyBassEditor::paint (juce::Graphics& g)
{
    g.fillAll (NinetyLook::dark);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a0a0a), 0.f, 0.f,
                                             NinetyLook::dark, 0.f, 120.f, false));
    g.fillRect (pHeader);

    // Logo + bass glow
    auto lr = juce::Rectangle<float> (14.f, 8.f, 80.f, 80.f);
    const float glow = juce::jlimit (0.f, 1.f, dOut);
    g.setGradientFill (juce::ColourGradient (NinetyLook::red.withAlpha (0.55f * glow), lr.getCentreX(), lr.getCentreY(),
                                             juce::Colours::transparentBlack, lr.getCentreX() + 70.f, lr.getCentreY(), true));
    g.fillEllipse (lr.expanded (30.f));
    g.drawImage (logo, lr, juce::RectanglePlacement::centred);

    g.setColour (juce::Colours::white);
    g.setFont (NinetyLook::font (38.f, true));
    g.drawText ("1NINETY-BASS", 110, 12, 480, 44, juce::Justification::centredLeft);
    g.setColour (NinetyLook::red);
    g.setFont (NinetyLook::font (13.f, true));
    g.drawText ("808 BASS BOOST  /  CRUNCH  /  SUB DESTROYER", 112, 56, 480, 20, juce::Justification::centredLeft);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (NinetyLook::font (12.f));
    g.drawText ("by taubenopa", 112, 74, 300, 18, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (NinetyLook::font (11.f, true));
    g.drawText ("PRESET", 610, 26, 100, 14, juce::Justification::centredLeft);
    g.drawText ("OVERSAMPLE", 826, 26, 80, 14, juce::Justification::centredLeft);

    g.setColour (NinetyLook::red.withAlpha (0.7f));
    g.fillRect (0, 95, getWidth(), 2);

    drawPanel (g, pInput, "INPUT / SPLIT");
    drawPanel (g, pLow,   "LOW BAND  -  808 SUB");
    drawPanel (g, pHigh,  "HIGH BAND  -  GRIT");
    drawPanel (g, pOut,   "OUTPUT");
    drawPanel (g, pMeter, "METERS");

    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (NinetyLook::font (10.f, true));
    g.drawText ("TYPE", pLow.getRight() - 190, pLow.getY() + 6, 36, 24, juce::Justification::centredRight);
    g.drawText ("TYPE", pHigh.getRight() - 160, pHigh.getY() + 6, 36, 24, juce::Justification::centredRight);

    auto m = pMeter.reduced (16, 0).withTrimmedTop (44).withTrimmedBottom (12);
    const float rowH = m.getHeight() / 3.f;
    drawMeter (g, juce::Rectangle<float> ((float) m.getX(), (float) m.getY(),          (float) m.getWidth(), rowH - 8.f), "IN",  dIn);
    drawMeter (g, juce::Rectangle<float> ((float) m.getX(), (float) m.getY() + rowH,   (float) m.getWidth(), rowH - 8.f), "LOW", dLow);
    drawMeter (g, juce::Rectangle<float> ((float) m.getX(), (float) m.getY() + 2*rowH, (float) m.getWidth(), rowH - 8.f), "OUT", dOut);

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.setFont (NinetyLook::font (11.f));
    g.drawText ("taubenopa  |  1ninety-bass  v1.0.0  |  VST3", 0, getHeight() - 36, getWidth(), 30,
                juce::Justification::centred);
}

void NinetyBassEditor::timerCallback()
{
    auto fall = [] (float& d, float v) { d = juce::jmax (v, d * 0.86f); };
    fall (dIn,  proc.meterIn.load());
    fall (dLow, proc.meterLow.load());
    fall (dOut, proc.meterOut.load());
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    repaint (pMeter);
    repaint (0, 0, 130, 96);
}
