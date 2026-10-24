#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Waveshapers
//==============================================================================
static inline float shape (int type, float x) noexcept
{
    switch (type)
    {
        case 0:  return std::tanh (x);                                            // Warm   (soft, smooth)
        case 1:  return x >= 0.f ? std::tanh (x)                                  // Tube   (asymmetric)
                                 : std::tanh (x * 0.7f) * 1.25f;
        case 2:  return x / (1.f + std::abs (x));                                 // Tape   (very soft knee)
        case 3:  return juce::jlimit (-1.f, 1.f, x);                              // Hard   (brutal clip)
        case 4:  return std::sin (x);                                             // Fold   (wavefolder)
        case 5:  return x >= 0.f ? 1.f - std::exp (-x)                            // Diode  (gritty, asymmetric)
                                 : -0.8f * (1.f - std::exp (1.5f * x));
        default: return x;
    }
}

static const juce::StringArray kShapeNames { "Warm", "Tube", "Tape", "Hard Clip", "Fold", "Diode" };

//==============================================================================
// Parameters
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout NinetyBassProcessor::createLayout()
{
    using namespace juce;
    using Attr = AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto F = [&] (const char* id, const char* name, float lo, float hi, float def,
                  const char* unit, float centre = 0.f)
    {
        NormalisableRange<float> r (lo, hi, 0.01f);
        if (centre > 0.f) r.setSkewForCentre (centre);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                                                            Attr().withLabel (unit)));
    };
    auto C = [&] (const char* id, const char* name, StringArray items, int def)
    {
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };
    auto B = [&] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    F (ID::inGain,   "Input",          -24.f,  24.f,  0.f,   " dB");
    B (ID::polarity, "Polarity Invert", false);
    F (ID::xover,    "Crossover",       60.f, 400.f, 140.f,  " Hz", 150.f);
    B (ID::monoSub,  "Mono Sub",        true);

    F (ID::boostDb,  "Sub Boost",        0.f,  24.f,  6.f,   " dB");
    F (ID::boostHz,  "Boost Freq",      25.f, 150.f, 52.f,   " Hz", 60.f);
    F (ID::boostQ,   "Boost Q",          0.3f,  4.f,  0.9f,  "");
    F (ID::tight,    "Tight (Low Cut)", 10.f,  80.f, 24.f,   " Hz", 25.f);
    F (ID::harm,     "Harmonics",        0.f, 100.f, 25.f,   " %");
    F (ID::lowDrive, "Crunch Drive",     0.f,  36.f, 12.f,   " dB");
    C (ID::lowType,  "Crunch Type",    kShapeNames, 1);
    F (ID::lowBias,  "Bias / Asym",     -1.f,   1.f,  0.f,   "");
    F (ID::lowCeil,  "Low Clip Ceiling",-24.f,  0.f, -1.f,   " dB");
    F (ID::lowLevel, "Low Level",      -24.f,  12.f,  0.f,   " dB");
    F (ID::punch,    "Punch",          -100.f, 100.f, 20.f,  " %");

    F (ID::clickDb,    "Click / Presence", 0.f, 15.f,  3.f,  " dB");
    F (ID::highDrive,  "Grit Drive",       0.f, 30.f,  6.f,  " dB");
    C (ID::highType,   "Grit Type",     kShapeNames, 5);
    F (ID::highLevel,  "High Level",     -48.f,  6.f, -6.f,  " dB");

    F (ID::tone,    "Tone (Hi Cut)",   1000.f, 20000.f, 16000.f, " Hz", 5000.f);
    F (ID::mix,     "Mix",                0.f, 100.f, 100.f,  " %");
    F (ID::outGain, "Output",           -24.f,  12.f,  0.f,   " dB");
    F (ID::ceiling, "Limiter Ceiling",  -12.f,   0.f, -0.3f,  " dB");
    B (ID::limiter, "Limiter",           true);
    C (ID::os,      "Oversampling",   { "1x", "2x", "4x" }, 1);
    B (ID::bypass,  "Bypass",            false);

    return { p.begin(), p.end() };
}

//==============================================================================
// Factory presets
//==============================================================================
namespace
{
    struct PV { const char* id; float v; };
    struct Preset { const char* name; std::vector<PV> values; };

    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> list
        {
            { "Init",
              { {ID::boostDb,6}, {ID::boostHz,52}, {ID::boostQ,0.9f}, {ID::tight,24}, {ID::harm,25},
                {ID::lowDrive,12}, {ID::lowType,1}, {ID::lowBias,0}, {ID::lowCeil,-1}, {ID::lowLevel,0},
                {ID::punch,20}, {ID::clickDb,3}, {ID::highDrive,6}, {ID::highType,5}, {ID::highLevel,-6},
                {ID::tone,16000}, {ID::mix,100}, {ID::outGain,0}, {ID::xover,140} } },

            { "Underground Crunch",
              { {ID::boostDb,9}, {ID::boostHz,48}, {ID::boostQ,1.1f}, {ID::tight,22}, {ID::harm,40},
                {ID::lowDrive,20}, {ID::lowType,5}, {ID::lowBias,0.25f}, {ID::lowCeil,-2}, {ID::lowLevel,-2},
                {ID::punch,35}, {ID::clickDb,5}, {ID::highDrive,12}, {ID::highType,5}, {ID::highLevel,-9},
                {ID::tone,12000}, {ID::mix,100}, {ID::outGain,-1}, {ID::xover,130} } },

            { "Hard Trap Distortion",
              { {ID::boostDb,12}, {ID::boostHz,55}, {ID::boostQ,0.8f}, {ID::tight,28}, {ID::harm,55},
                {ID::lowDrive,28}, {ID::lowType,3}, {ID::lowBias,0.1f}, {ID::lowCeil,-3}, {ID::lowLevel,-4},
                {ID::punch,60}, {ID::clickDb,8}, {ID::highDrive,18}, {ID::highType,3}, {ID::highLevel,-12},
                {ID::tone,9000}, {ID::mix,100}, {ID::outGain,-2}, {ID::xover,150} } },

            { "Blown Out Sub",
              { {ID::boostDb,15}, {ID::boostHz,45}, {ID::boostQ,1.4f}, {ID::tight,20}, {ID::harm,70},
                {ID::lowDrive,34}, {ID::lowType,4}, {ID::lowBias,-0.2f}, {ID::lowCeil,-4}, {ID::lowLevel,-6},
                {ID::punch,10}, {ID::clickDb,2}, {ID::highDrive,4}, {ID::highType,4}, {ID::highLevel,-18},
                {ID::tone,7000}, {ID::mix,100}, {ID::outGain,-3}, {ID::xover,110} } },

            { "Warm Tape 808",
              { {ID::boostDb,5}, {ID::boostHz,58}, {ID::boostQ,0.7f}, {ID::tight,26}, {ID::harm,20},
                {ID::lowDrive,9}, {ID::lowType,2}, {ID::lowBias,0.05f}, {ID::lowCeil,-1}, {ID::lowLevel,0},
                {ID::punch,10}, {ID::clickDb,2}, {ID::highDrive,0}, {ID::highType,2}, {ID::highLevel,-6},
                {ID::tone,18000}, {ID::mix,100}, {ID::outGain,0}, {ID::xover,160} } },

            { "Clean Sub Boost",
              { {ID::boostDb,8}, {ID::boostHz,50}, {ID::boostQ,0.8f}, {ID::tight,22}, {ID::harm,10},
                {ID::lowDrive,0}, {ID::lowType,0}, {ID::lowBias,0}, {ID::lowCeil,0}, {ID::lowLevel,0},
                {ID::punch,0}, {ID::clickDb,0}, {ID::highDrive,0}, {ID::highType,0}, {ID::highLevel,0},
                {ID::tone,20000}, {ID::mix,100}, {ID::outGain,0}, {ID::xover,140} } },
        };
        return list;
    }
}

int NinetyBassProcessor::getNumPrograms() { return (int) presets().size(); }
const juce::String NinetyBassProcessor::getProgramName (int i) { return presets()[(size_t) i].name; }

void NinetyBassProcessor::setCurrentProgram (int index)
{
    index = juce::jlimit (0, getNumPrograms() - 1, index);
    currentProgram = index;
    for (auto& pv : presets()[(size_t) index].values)
        if (auto* prm = apvts.getParameter (pv.id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (pv.v));
}

//==============================================================================
NinetyBassProcessor::NinetyBassProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < 3; ++i)
    {
        osLow[i]  = std::make_unique<juce::dsp::Oversampling<float>> (
            2, (size_t) i, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        osHigh[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            2, (size_t) i, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    }
}

bool NinetyBassProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void NinetyBassProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (samplesPerBlock, 64);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 2 };

    crossover.prepare (spec);
    crossover.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    tightHpf.prepare (spec);  boostPeak.prepare (spec);  clickPeak.prepare (spec);
    toneLpf.prepare (spec);   dcBlock.prepare (spec);
    limiter.prepare (spec);   limiter.setRelease (80.f);
    dryDelay.prepare (spec);

    *dcBlock.state = *Coeffs::makeHighPass (sr, 10.f, 0.7071f);

    for (int i = 0; i < 3; ++i)
    {
        osLow[i]->initProcessing ((size_t) maxBlock);  osLow[i]->reset();
        osHigh[i]->initProcessing ((size_t) maxBlock); osHigh[i]->reset();
    }

    lowBuf.setSize  (2, maxBlock);
    highBuf.setSize (2, maxBlock);
    dryBuf.setSize  (2, maxBlock);

    sInGain.reset  (sr, 0.03);  sInGain.setCurrentAndTargetValue (1.f);
    sOutGain.reset (sr, 0.03);  sOutGain.setCurrentAndTargetValue (1.f);
    sMix.reset     (sr, 0.03);  sMix.setCurrentAndTargetValue (1.f);

    cTight = cBoostHz = cBoostQ = cClick = cTone = -1.f;  cBoostDb = -999.f;
    std::fill (std::begin (envFast), std::end (envFast), 0.f);
    std::fill (std::begin (envSlow), std::end (envSlow), 0.f);
    lastLatency = -1;
}

//==============================================================================
void NinetyBassProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // process in chunks, falls ein Host groessere Bloecke als angekuendigt liefert
    int pos = 0;
    const int total = buffer.getNumSamples();
    while (pos < total)
    {
        const int len = juce::jmin (maxBlock, total - pos);
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), numCh, pos, len);
        processChunk (chunk);
        pos += len;
    }
}

void NinetyBassProcessor::processChunk (juce::AudioBuffer<float>& buffer)
{
    const int numCh = buffer.getNumChannels();
    const int n     = buffer.getNumSamples();

    const int osIdx = juce::jlimit (0, 2, (int) get (ID::os));
    auto& oL = *osLow[osIdx];
    auto& oH = *osHigh[osIdx];

    const int latency = juce::roundToInt (oL.getLatencyInSamples());
    if (latency != lastLatency)
    {
        lastLatency = latency;
        setLatencySamples (latency);
        dryDelay.setDelay ((float) latency);
    }

    //---- Bypass (zeitkorrekt ueber die Delayline)
    if (get (ID::bypass) > 0.5f)
    {
        for (int i = 0; i < n; ++i)
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto* d = buffer.getWritePointer (ch);
                dryDelay.pushSample (ch, d[i]);
                d[i] = dryDelay.popSample (ch);
            }
        return;
    }

    for (int ch = 0; ch < numCh; ++ch)
        dryBuf.copyFrom (ch, 0, buffer, ch, 0, n);

    meterIn.store (buffer.getMagnitude (0, n));

    //---- Input gain + polarity
    const float pol = get (ID::polarity) > 0.5f ? -1.f : 1.f;
    sInGain.setTargetValue (pol * juce::Decibels::decibelsToGain (get (ID::inGain)));
    for (int i = 0; i < n; ++i)
    {
        const float g = sInGain.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }

    //---- Crossover (Linkwitz-Riley 4th order)
    crossover.setCutoffFrequency (get (ID::xover));
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* in = buffer.getReadPointer (ch);
        auto* lo = lowBuf.getWritePointer (ch);
        auto* hi = highBuf.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            crossover.processSample (ch, in[i], lo[i], hi[i]);
    }

    if (numCh == 2 && get (ID::monoSub) > 0.5f)
    {
        auto* l = lowBuf.getWritePointer (0);
        auto* r = lowBuf.getWritePointer (1);
        for (int i = 0; i < n; ++i) { const float m = 0.5f * (l[i] + r[i]); l[i] = r[i] = m; }
    }

    juce::dsp::AudioBlock<float> lowBlock  (lowBuf.getArrayOfWritePointers(),  (size_t) numCh, (size_t) n);
    juce::dsp::AudioBlock<float> highBlock (highBuf.getArrayOfWritePointers(), (size_t) numCh, (size_t) n);

    //==========================  LOW BAND  ==========================
    {
        const float tight = get (ID::tight);
        if (tight != cTight) { cTight = tight; *tightHpf.state = *Coeffs::makeHighPass (sr, tight, 0.7071f); }

        const float bHz = get (ID::boostHz), bQ = get (ID::boostQ), bDb = get (ID::boostDb);
        if (bHz != cBoostHz || bQ != cBoostQ || bDb != cBoostDb)
        {
            cBoostHz = bHz; cBoostQ = bQ; cBoostDb = bDb;
            *boostPeak.state = *Coeffs::makePeakFilter (sr, bHz, bQ, juce::Decibels::decibelsToGain (bDb));
        }

        tightHpf.process  (juce::dsp::ProcessContextReplacing<float> (lowBlock));
        boostPeak.process (juce::dsp::ProcessContextReplacing<float> (lowBlock));

        // Punch = Transient-Shaper (Attack-Betonung / -Absenkung)
        const float punch = get (ID::punch) * 0.01f;
        if (std::abs (punch) > 0.001f)
        {
            const float aF = 1.f - std::exp (-1.f / (float) (sr * 0.0005));
            const float rF = 1.f - std::exp (-1.f / (float) (sr * 0.010));
            const float aS = 1.f - std::exp (-1.f / (float) (sr * 0.015));
            const float rS = 1.f - std::exp (-1.f / (float) (sr * 0.060));
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto* d = lowBuf.getWritePointer (ch);
                float f = envFast[ch], s = envSlow[ch];
                for (int i = 0; i < n; ++i)
                {
                    const float a = std::abs (d[i]);
                    f += (a - f) * (a > f ? aF : rF);
                    s += (a - s) * (a > s ? aS : rS);
                    const float diff = juce::jlimit (-1.f, 1.f, (f - s) / (s + 1.0e-4f));
                    d[i] *= juce::jlimit (0.25f, 3.f, 1.f + punch * diff * 0.9f);
                }
                envFast[ch] = f; envSlow[ch] = s;
            }
        }

        // Saturation @ oversampled rate
        const int   type    = (int) get (ID::lowType);
        const float driveLn = juce::Decibels::decibelsToGain (get (ID::lowDrive));
        const float comp    = 1.f / std::sqrt (driveLn);
        const float bias    = get (ID::lowBias) * 0.8f;
        const float biasOff = shape (type, bias);
        const float harm    = get (ID::harm) * 0.01f;
        const float ceil    = juce::Decibels::decibelsToGain (get (ID::lowCeil));
        const float level   = juce::Decibels::decibelsToGain (get (ID::lowLevel));

        auto up = oL.processSamplesUp (juce::dsp::AudioBlock<const float> (lowBlock));
        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            auto* d = up.getChannelPointer (ch);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                const float x = d[i];
                float y = (shape (type, x * driveLn + bias) - biasOff) * comp;
                y += harm * (0.8f * x * x + 0.4f * x * x * x);   // 2nd/3rd harmonics (DC wird spaeter entfernt)
                y = juce::jlimit (-ceil, ceil, y);
                d[i] = y * level;
            }
        }
        oL.processSamplesDown (lowBlock);
        meterLow.store (lowBuf.getMagnitude (0, n));
    }

    //==========================  HIGH BAND  =========================
    {
        const float cDb = get (ID::clickDb);
        if (cDb != cClick) { cClick = cDb; *clickPeak.state = *Coeffs::makePeakFilter (sr, 3200.f, 0.8f, juce::Decibels::decibelsToGain (cDb)); }
        clickPeak.process (juce::dsp::ProcessContextReplacing<float> (highBlock));

        const float dDb   = get (ID::highDrive);
        const int   type  = (int) get (ID::highType);
        const float level = juce::Decibels::decibelsToGain (get (ID::highLevel));

        auto up = oH.processSamplesUp (juce::dsp::AudioBlock<const float> (highBlock));
        const bool sat = dDb > 0.05f;
        const float driveLn = juce::Decibels::decibelsToGain (dDb);
        const float comp = 1.f / std::sqrt (driveLn);
        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            auto* d = up.getChannelPointer (ch);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                float y = d[i];
                if (sat) y = shape (type, y * driveLn) * comp;
                d[i] = y * level;
            }
        }
        oH.processSamplesDown (highBlock);
    }

    //---- Summe
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* o = buffer.getWritePointer (ch);
        auto* l = lowBuf.getReadPointer (ch);
        auto* h = highBuf.getReadPointer (ch);
        for (int i = 0; i < n; ++i) o[i] = l[i] + h[i];
    }

    juce::dsp::AudioBlock<float> outBlock (buffer);

    //---- Tone, DC-Filter
    const float tone = juce::jmin (get (ID::tone), (float) sr * 0.45f);
    if (tone != cTone) { cTone = tone; *toneLpf.state = *Coeffs::makeLowPass (sr, tone, 0.7071f); }
    toneLpf.process (juce::dsp::ProcessContextReplacing<float> (outBlock));
    dcBlock.process (juce::dsp::ProcessContextReplacing<float> (outBlock));

    //---- Output gain
    sOutGain.setTargetValue (juce::Decibels::decibelsToGain (get (ID::outGain)));
    for (int i = 0; i < n; ++i)
    {
        const float g = sOutGain.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }

    //---- Limiter
    if (get (ID::limiter) > 0.5f)
    {
        limiter.setThreshold (get (ID::ceiling));
        limiter.process (juce::dsp::ProcessContextReplacing<float> (outBlock));
    }

    //---- Dry/Wet (Dry latenzkorrigiert)
    sMix.setTargetValue (get (ID::mix) * 0.01f);
    for (int i = 0; i < n; ++i)
    {
        const float m = sMix.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
        {
            dryDelay.pushSample (ch, dryBuf.getReadPointer (ch)[i]);
            const float dry = dryDelay.popSample (ch);
            auto* o = buffer.getWritePointer (ch);
            o[i] = dry * (1.f - m) + o[i] * m;
        }
    }

    meterOut.store (buffer.getMagnitude (0, n));
}

//==============================================================================
juce::AudioProcessorEditor* NinetyBassProcessor::createEditor() { return new NinetyBassEditor (*this); }

void NinetyBassProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void NinetyBassProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NinetyBassProcessor(); }
