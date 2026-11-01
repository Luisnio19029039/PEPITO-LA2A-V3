#pragma once
#include <JuceHeader.h>

class PepitoLA2AAudioProcessor : public juce::AudioProcessor
{
public:
    PepitoLA2AAudioProcessor();
    ~PepitoLA2AAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "PEPITO LA2A"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    float getGainReductionDb() const noexcept { return gainReductionDb.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::atomic<float> gainReductionDb { 0.0f };
    float envelope = 0.0f;
    float releaseEnvelope = 0.0f;
    double currentSampleRate = 44100.0;
    float attackCoeff = 0.0f;
    float fastReleaseCoeff = 0.0f;
    float slowReleaseCoeff = 0.0f;
    float longReleaseCoeff = 0.0f;
    float grFast = 0.0f;
    float grSlow = 0.0f;
    float grSmooth = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PepitoLA2AAudioProcessor)
};
