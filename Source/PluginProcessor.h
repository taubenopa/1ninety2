#pragma once
#include <JuceHeader.h>

namespace ID
{
    // global
    inline constexpr const char* inGain   = "inGain";
    inline constexpr const char* polarity = "polarity";
    inline constexpr const char* xover    = "xover";
    inline constexpr const char* monoSub  = "monoSub";
    // low band (808 sub)
    inline constexpr const char* boostDb  = "boostDb";
    inline constexpr const char* boostHz  = "boostHz";
    inline constexpr const char* boostQ   = "boostQ";
    inline constexpr const char* tight    = "tight";
    inline constexpr const char* harm     = "harm";
    inline constexpr const char* lowDrive = "lowDrive";
    inline constexpr const char* lowType  = "lowType";
    inline constexpr const char* lowBias  = "lowBias";
    inline constexpr const char* lowCeil  = "lowCeil";
    inline constexpr const char* lowLevel = "lowLevel";
    inline constexpr const char* punch    = "punch";
    // high band (crunch / click)
    inline constexpr const char* clickDb    = "clickDb";
    inline constexpr const char* highDrive  = "highDrive";
    inline constexpr const char* highType   = "highType";
    inline constexpr const char* highLevel  = "highLevel";
    // output
    inline constexpr const char* tone     = "tone";
    inline constexpr const char* mix      = "mix";
    inline constexpr const char* outGain  = "outGain";
    inline constexpr const char* ceiling  = "ceiling";
    inline constexpr const char* limiter  = "limiter";
    inline constexpr const char* os       = "os";
    inline constexpr const char* bypass   = "bypass";
}

class NinetyBassProcessor : public juce::AudioProcessor
{
public:
    NinetyBassProcessor();
    ~NinetyBassProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "1ninety-bass"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> meterIn { 0.f }, meterLow { 0.f }, meterOut { 0.f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void processChunk (juce::AudioBuffer<float>& buffer);
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    using IIR = juce::dsp::IIR::Filter<float>;
    using Coeffs = juce::dsp::IIR::Coefficients<float>;
    using Duplicator = juce::dsp::ProcessorDuplicator<IIR, Coeffs>;

    double sr = 44100.0;
    int maxBlock = 512;
    int currentProgram = 0;
    int lastLatency = -1;

    juce::dsp::LinkwitzRileyFilter<float> crossover;
    Duplicator tightHpf, boostPeak, clickPeak, toneLpf, dcBlock;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 4096 };

    std::unique_ptr<juce::dsp::Oversampling<float>> osLow[3], osHigh[3];

    juce::AudioBuffer<float> lowBuf, highBuf, dryBuf;
    float envFast[2] {}, envSlow[2] {};

    juce::SmoothedValue<float> sInGain, sOutGain, sMix;

    // cached filter settings to avoid re-allocating coefficients every block
    float cTight = -1.f, cBoostHz = -1.f, cBoostQ = -1.f, cBoostDb = -999.f,
          cClick = -999.f, cTone = -1.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NinetyBassProcessor)
};
