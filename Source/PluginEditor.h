#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
struct NinetyLook : public juce::LookAndFeel_V4
{
    NinetyLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getLabelFont (juce::Label&) override;

    static juce::Font font (float h, bool bold = false);
    static inline const juce::Colour red   { 0xffe01818 };
    static inline const juce::Colour dark  { 0xff0d0a0a };
    static inline const juce::Colour panel { 0xff171212 };
};

//==============================================================================
class NinetyBassEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NinetyBassEditor (NinetyBassProcessor&);
    ~NinetyBassEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob : public juce::Component
    {
        Knob (juce::AudioProcessorValueTreeState&, const char* id, const juce::String& name);
        void resized() override;
        juce::Label label;
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    Knob* addKnob (const char* id, const juce::String& name);
    void addToggle (juce::ToggleButton& b, const char* id, const juce::String& text);
    void addCombo (juce::ComboBox& c, const char* id, const juce::StringArray& items);
    void timerCallback() override;
    void drawMeter (juce::Graphics&, juce::Rectangle<float>, const juce::String&, float level);
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String&);

    NinetyBassProcessor& proc;
    NinetyLook look;
    juce::Image logo;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::map<juce::String, Knob*> k;

    juce::ComboBox presetBox, lowTypeBox, highTypeBox, osBox;
    juce::ToggleButton bypassBtn, polarityBtn, monoBtn, limiterBtn;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> comboAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttonAtts;

    juce::Rectangle<int> pInput, pLow, pHigh, pOut, pMeter, pHeader;
    float dIn = 0.f, dLow = 0.f, dOut = 0.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NinetyBassEditor)
};
