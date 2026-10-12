#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include <vector>

class LoFiProcessor : public juce::AudioProcessor
{
public:
    LoFiProcessor();
    ~LoFiProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LoFi Machine"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Biquad
    {
        float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f, z1 = 0.f, z2 = 0.f;

        void reset() { z1 = z2 = 0.f; }

        float process (float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }

        void setLowpass (double sr, double f, double q);
        void setHighpass (double sr, double f, double q);
        void setPeak (double sr, double f, double q, double gainDb);
    };

    std::atomic<float> *pBits = nullptr, *pRate = nullptr, *pFType = nullptr,
                       *pCutoff = nullptr, *pReso = nullptr, *pDrive = nullptr,
                       *pWow = nullptr, *pFlutter = nullptr, *pMix = nullptr, *pOut = nullptr,
                       *pTone = nullptr, *pHiss = nullptr, *pDrop = nullptr, *pStereo = nullptr;

    Biquad filt[2];
    Biquad toneLpF[2], bumpF[2];
    std::array<std::vector<float>, 2> delayBuf;
    int bufSize = 0, writePos = 0, baseDelay = 0;
    double sr = 44100.0;

    float wowPhase = 0.f, flutPhase = 0.f, randState = 0.f, randTarget = 0.f;
    int randCounter = 0;
    float azPhase = 0.f, dropGain = 1.f, dropTarget = 1.f, lastTone = -1.f;
    int dropRemain = 0;
    float prevNoise[2] = { 0.f, 0.f };
    float hold[2] = { 0.f, 0.f };
    float srPhase[2] = { 1.f, 1.f };
    float tapeLp[2] = { 0.f, 0.f };

    int lastType = -1;
    float lastCutoff = -1.f, lastQ = -1.f;

    juce::SmoothedValue<float> driveSm, mixSm, outSm;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoFiProcessor)
};
