#include "PluginProcessor.h"
#include <cmath>

namespace
{
    constexpr float kTwoPi = 6.28318530718f;
}

void LoFiProcessor::Biquad::setLowpass (double sr, double f, double q)
{
    const double w0 = 6.283185307179586 * f / sr;
    const double cw = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = (float) (((1.0 - cw) * 0.5) / a0);
    b1 = (float) ((1.0 - cw) / a0);
    b2 = b0;
    a1 = (float) ((-2.0 * cw) / a0);
    a2 = (float) ((1.0 - alpha) / a0);
}

void LoFiProcessor::Biquad::setHighpass (double sr, double f, double q)
{
    const double w0 = 6.283185307179586 * f / sr;
    const double cw = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    b0 = (float) (((1.0 + cw) * 0.5) / a0);
    b1 = (float) (-(1.0 + cw) / a0);
    b2 = b0;
    a1 = (float) ((-2.0 * cw) / a0);
    a2 = (float) ((1.0 - alpha) / a0);
}

void LoFiProcessor::Biquad::setPeak (double sr, double f, double q, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 6.283185307179586 * f / sr;
    const double cw = std::cos (w0);
    const double alpha = std::sin (w0) / (2.0 * q);
    const double a0 = 1.0 + alpha / A;
    b0 = (float) ((1.0 + alpha * A) / a0);
    b1 = (float) ((-2.0 * cw) / a0);
    b2 = (float) ((1.0 - alpha * A) / a0);
    a1 = b1;
    a2 = (float) ((1.0 - alpha / A) / a0);
}

juce::AudioProcessorValueTreeState::ParameterLayout LoFiProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    l.add (std::make_unique<AudioParameterInt> (ParameterID { "bits", 1 }, "Bits (8-24)", 8, 24, 12));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "rate", 1 }, "Sample Rate (Hz)",
           NormalisableRange<float> (1000.f, 40000.f, 1.f, 0.4f), 20000.f));

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "ftype", 1 }, "Filter Type",
           StringArray { "Off", "Low-pass", "High-pass" }, 1));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Filter Cutoff (Hz)",
           NormalisableRange<float> (20.f, 20000.f, 1.f, 0.3f), 8000.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "reso", 1 }, "Filter Resonance",
           NormalisableRange<float> (0.5f, 4.f, 0.01f), 0.71f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Tape Saturation (%)",
           NormalisableRange<float> (0.f, 100.f, 0.1f), 30.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "wow", 1 }, "Wow (%)",
           NormalisableRange<float> (0.f, 100.f, 0.1f), 20.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "flutter", 1 }, "Flutter (%)",
           NormalisableRange<float> (0.f, 100.f, 0.1f), 15.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tape Tone - Dark + Bass Bump (%)",
           NormalisableRange<float> (0.f, 100.f, 0.1f), 40.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "hiss", 1 }, "Tape Hiss (%)",
           NormalisableRange<float> (0.f, 100.f, 0.1f), 15.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "drop", 1 }, "Dropouts (%)",
           NormalisableRange<float> (0.f, 100.f, 0.1f), 10.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "stereo", 1 }, "Stereo Drift (%)",
           NormalisableRange<float> (0.f, 100.f, 0.1f), 15.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix (%)",
           NormalisableRange<float> (0.f, 100.f, 0.1f), 100.f));

    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "out", 1 }, "Output (dB)",
           NormalisableRange<float> (-24.f, 12.f, 0.1f), 0.f));

    return l;
}

LoFiProcessor::LoFiProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pBits    = apvts.getRawParameterValue ("bits");
    pRate    = apvts.getRawParameterValue ("rate");
    pFType   = apvts.getRawParameterValue ("ftype");
    pCutoff  = apvts.getRawParameterValue ("cutoff");
    pReso    = apvts.getRawParameterValue ("reso");
    pDrive   = apvts.getRawParameterValue ("drive");
    pWow     = apvts.getRawParameterValue ("wow");
    pFlutter = apvts.getRawParameterValue ("flutter");
    pMix     = apvts.getRawParameterValue ("mix");
    pOut     = apvts.getRawParameterValue ("out");
    pTone    = apvts.getRawParameterValue ("tone");
    pHiss    = apvts.getRawParameterValue ("hiss");
    pDrop    = apvts.getRawParameterValue ("drop");
    pStereo  = apvts.getRawParameterValue ("stereo");
}

bool LoFiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void LoFiProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    baseDelay = (int) std::round (0.008 * sr);
    bufSize = (int) (0.025 * sr) + 8;

    for (auto& b : delayBuf)
        b.assign ((size_t) bufSize, 0.f);

    writePos = 0;
    wowPhase = flutPhase = randState = randTarget = 0.f;
    randCounter = 0;

    for (int c = 0; c < 2; ++c)
    {
        hold[c] = 0.f;
        srPhase[c] = 1.f;
        tapeLp[c] = 0.f;
        filt[c].reset();
        toneLpF[c].reset();
        bumpF[c].reset();
        prevNoise[c] = 0.f;
    }
    lastType = -1;
    lastTone = -1.f;
    azPhase = 0.f;
    dropGain = dropTarget = 1.f;
    dropRemain = 0;

    driveSm.reset (sr, 0.03);
    mixSm.reset (sr, 0.03);
    outSm.reset (sr, 0.03);
    driveSm.setCurrentAndTargetValue (pDrive->load() * 0.01f);
    mixSm.setCurrentAndTargetValue (pMix->load() * 0.01f);
    outSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));

    setLatencySamples (baseDelay);
}

void LoFiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    const float fsr = (float) sr;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    // ---- parâmetros do bloco ----
    const int bits = (int) pBits->load();
    const float rate = pRate->load();
    const bool doBits = bits < 24;
    const bool doRate = rate < fsr - 1.f;
    const float levels = (float) (1 << (bits - 1));
    const float srInc = rate / fsr;

    const int ftype = (int) pFType->load();
    const float cutoff = pCutoff->load();
    const float q = pReso->load();
    if (ftype != lastType || cutoff != lastCutoff || q != lastQ)
    {
        lastType = ftype;
        lastCutoff = cutoff;
        lastQ = q;
        if (ftype > 0)
        {
            const float fc = juce::jmin (cutoff, 0.45f * fsr);
            for (auto& f : filt)
            {
                if (ftype == 1) f.setLowpass (sr, fc, q);
                else            f.setHighpass (sr, fc, q);
            }
        }
    }

    const float driveParam = pDrive->load() * 0.01f;
    driveSm.setTargetValue (driveParam);
    mixSm.setTargetValue (pMix->load() * 0.01f);
    outSm.setTargetValue (juce::Decibels::decibelsToGain (pOut->load()));

    const float fcTape = juce::jmin (16000.f - 9000.f * driveParam, 0.45f * fsr);
    const float tapeA = 1.f - std::exp (-kTwoPi * fcTape / fsr);

    const float tone = pTone->load() * 0.01f;
    if (tone != lastTone)
    {
        lastTone = tone;
        const float fcTone = juce::jmin (18000.f - 15000.f * tone, 0.45f * fsr);
        for (int c = 0; c < 2; ++c)
        {
            toneLpF[c].setLowpass (sr, fcTone, 0.6);
            bumpF[c].setPeak (sr, 90.0, 0.8, 4.0 * tone);
        }
    }

    const float hissRaw = pHiss->load() * 0.01f;
    const float hissAmp = hissRaw * hissRaw * 0.03f;
    const float dropAmt = pDrop->load() * 0.01f;
    const float dropProb = (dropAmt * 2.0f) / fsr;
    const float dropCoef = 1.f - std::exp (-1.f / (0.004f * fsr));
    const float stereoDepth = pStereo->load() * 0.01f;
    const float azMax = 0.0005f * fsr;
    const float azInc = 0.23f / fsr;

    const float wowDepth = pWow->load() * 0.01f;
    const float flutDepth = pFlutter->load() * 0.01f;
    const float wowMax = 0.004f * fsr;
    const float flutMax = 0.0004f * fsr;
    const float wowInc = 0.6f / fsr;
    const float flutInc = 7.0f / fsr;
    const float randSlew = 1.f / (0.4f * fsr);
    const int randPeriod = (int) (0.25 * sr);

    for (int i = 0; i < n; ++i)
    {
        const float drive = driveSm.getNextValue();
        const float mix = mixSm.getNextValue();
        const float outG = outSm.getNextValue();

        wowPhase += wowInc;   if (wowPhase >= 1.f)  wowPhase -= 1.f;
        flutPhase += flutInc; if (flutPhase >= 1.f) flutPhase -= 1.f;

        if (--randCounter <= 0)
        {
            randTarget = rng.nextFloat() * 2.f - 1.f;
            randCounter = randPeriod;
        }
        randState += (randTarget - randState) * randSlew;

        azPhase += azInc; if (azPhase >= 1.f) azPhase -= 1.f;
        const float azMod = std::sin (kTwoPi * azPhase);

        if (dropRemain > 0)
        {
            if (--dropRemain == 0)
                dropTarget = 1.f;
        }
        else if (dropAmt > 0.f && rng.nextFloat() < dropProb)
        {
            dropTarget = 0.2f + 0.4f * rng.nextFloat();
            dropRemain = (int) ((0.02f + 0.1f * rng.nextFloat()) * fsr);
        }
        dropGain += (dropTarget - dropGain) * dropCoef;

        const float wowMod = 0.75f * std::sin (kTwoPi * wowPhase) + 0.25f * randState;
        const float flutMod = std::sin (kTwoPi * flutPhase);
        const float d = (float) baseDelay + wowDepth * wowMax * wowMod + flutDepth * flutMax * flutMod;

        const float gain = 1.f + 8.f * drive;
        const float bias = 0.15f * drive;
        const float tanhBias = std::tanh (bias);
        const float tanhGain = std::tanh (gain);
        const float satAmt = juce::jmin (1.f, drive * 5.f);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            auto& buf = delayBuf[(size_t) ch];

            buf[(size_t) writePos] = data[i];

            // dry com a mesma latência do caminho do wow/flutter
            int dryIdx = writePos - baseDelay;
            if (dryIdx < 0) dryIdx += bufSize;
            const float dry = buf[(size_t) dryIdx];

            // wow/flutter: leitura com delay modulado
            const float dCh = (ch == 1) ? d + stereoDepth * azMax * azMod : d;
            float rp = (float) writePos - dCh;
            while (rp < 0.f) rp += (float) bufSize;
            int i0 = (int) rp;
            const float fr = rp - (float) i0;
            if (i0 >= bufSize) i0 -= bufSize;
            int i1 = i0 + 1;
            if (i1 >= bufSize) i1 = 0;
            float s = buf[(size_t) i0] * (1.f - fr) + buf[(size_t) i1] * fr;

            // saturação de fita (assimétrica + leve perda de agudos)
            const float sat = (std::tanh (gain * s + bias) - tanhBias) / tanhGain;
            s = s + satAmt * (sat - s);
            tapeLp[ch] += tapeA * (s - tapeLp[ch]);
            s = s + satAmt * (tapeLp[ch] - s);

            // redução de sample rate (sample & hold)
            if (doRate)
            {
                srPhase[ch] += srInc;
                if (srPhase[ch] >= 1.f)
                {
                    srPhase[ch] -= 1.f;
                    hold[ch] = s;
                }
                s = hold[ch];
            }

            // redução de bits
            if (doBits)
            {
                const float c = juce::jlimit (-1.f, 1.f, s);
                s = std::round (c * levels) / levels;
            }

            // tom de fita: escurece agudos + bump nos graves
            s = bumpF[ch].process (toneLpF[ch].process (s));

            // filtro
            if (ftype > 0)
                s = filt[ch].process (s);

            // dropouts e chiado de fita
            s *= dropGain;
            const float nz = rng.nextFloat() * 2.f - 1.f;
            s += (nz - prevNoise[ch]) * 0.5f * hissAmp;
            prevNoise[ch] = nz;

            data[i] = (dry + mix * (s - dry)) * outG;
        }

        if (++writePos >= bufSize)
            writePos = 0;
    }
}

juce::AudioProcessorEditor* LoFiProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void LoFiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void LoFiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoFiProcessor();
}
