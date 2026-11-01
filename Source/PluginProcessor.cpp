#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr auto peakID = "peakReduction";
    constexpr auto gainID = "gain";
    constexpr auto limitID = "limit";

    float dbToGain (float db) noexcept
    {
        return juce::Decibels::decibelsToGain (db);
    }

    float softKneeReduction (float levelDb, float thresholdDb, float ratio, float kneeDb) noexcept
    {
        const float over = levelDb - thresholdDb;
        if (over <= -0.5f * kneeDb)
            return 0.0f;

        const float slope = 1.0f - (1.0f / ratio);
        if (over >= 0.5f * kneeDb)
            return over * slope;

        const float x = over + 0.5f * kneeDb;
        return slope * x * x / (2.0f * kneeDb);
    }
}

PepitoLA2AAudioProcessor::PepitoLA2AAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PEPITO_STATE", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
PepitoLA2AAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<juce::AudioParameterFloat>(
        peakID, "Peak Reduction", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 3.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat>(
        gainID, "Gain", juce::NormalisableRange<float> (0.0f, 10.0f, 0.01f), 5.0f));

    p.push_back (std::make_unique<juce::AudioParameterBool>(
        limitID, "Limit", false));

    return { p.begin(), p.end() };
}

void PepitoLA2AAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = juce::jmax (sampleRate, 1.0);
    // Optical-style timing: moderately quick attack and program-dependent release.
    attackCoeff      = std::exp (-1.0f / (0.010f * static_cast<float> (currentSampleRate)));
    fastReleaseCoeff = std::exp (-1.0f / (0.075f * static_cast<float> (currentSampleRate)));
    slowReleaseCoeff = std::exp (-1.0f / (0.650f * static_cast<float> (currentSampleRate)));
    longReleaseCoeff = std::exp (-1.0f / (5.0f * static_cast<float> (currentSampleRate)));
    envelope = releaseEnvelope = grFast = grSlow = grSmooth = 0.0f;
    gainReductionDb.store (0.0f);
}

void PepitoLA2AAudioProcessor::releaseResources() {}

bool PepitoLA2AAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return in == out && (out == juce::AudioChannelSet::mono()
                      || out == juce::AudioChannelSet::stereo());
}

void PepitoLA2AAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    auto* peakParam = parameters.getRawParameterValue (peakID);
    auto* gainParam = parameters.getRawParameterValue (gainID);
    auto* limitParam = parameters.getRawParameterValue (limitID);

    const float peak = peakParam->load();
    const float gain = gainParam->load();
    const bool limiting = limitParam->load() > 0.5f;

    // LA-2A-inspired optical behaviour: a soft-knee T4-style detector,
    // fixed programme-dependent recovery and no 1176-style attack/release controls.
    const float thresholdDb = juce::jmap (peak, 0.0f, 10.0f, 0.0f, -32.0f);
    const float ratio = limiting ? 10.0f : 3.5f;
    const float outputGainDb = juce::jmap (gain, 0.0f, 10.0f, -6.0f, 18.0f);
    const float outputGain = dbToGain (outputGainDb);

    for (int i = 0; i < samples; ++i)
    {
        float detector = 0.0f;
        for (int ch = 0; ch < channels; ++ch)
            detector = juce::jmax (detector, std::abs (buffer.getReadPointer (ch)[i]));

        // Optical cell: quick programme attack, followed by a slower memory tail.
        if (detector > envelope)
            envelope = attackCoeff * envelope + (1.0f - attackCoeff) * detector;
        else
            envelope = fastReleaseCoeff * envelope + (1.0f - fastReleaseCoeff) * detector;

        releaseEnvelope = slowReleaseCoeff * releaseEnvelope
                        + (1.0f - slowReleaseCoeff) * envelope;
        const float opticalLevel = 0.62f * envelope + 0.38f * releaseEnvelope;
        const float detectorDb = juce::Decibels::gainToDecibels (juce::jmax (opticalLevel, 1.0e-7f));
        const float targetReduction = softKneeReduction (detectorDb, thresholdDb, ratio, 6.0f);

        // LA-2A-like two-stage recovery: an initial quick return and a
        // programme-dependent long tail (roughly 0.65–5 seconds).
        if (targetReduction > grFast)
            grFast = attackCoeff * grFast + (1.0f - attackCoeff) * targetReduction;
        else
            grFast = fastReleaseCoeff * grFast + (1.0f - fastReleaseCoeff) * targetReduction;

        if (targetReduction > grSlow)
            grSlow = attackCoeff * grSlow + (1.0f - attackCoeff) * targetReduction;
        else
        {
            const float recovery = juce::jlimit (0.0f, 1.0f, grSlow / 10.0f);
            const float programmeRelease = juce::jmap (recovery, slowReleaseCoeff, longReleaseCoeff);
            grSlow = programmeRelease * grSlow + (1.0f - programmeRelease) * targetReduction;
        }

        grSmooth = 0.55f * grFast + 0.45f * grSlow;
        const float compressorGain = dbToGain (-grSmooth);

        for (int ch = 0; ch < channels; ++ch)
        {
            float x = buffer.getReadPointer (ch)[i] * compressorGain * outputGain;
            // Gentle transformer/valve-style rounding; deliberately not a hard limiter.
            const float drive = limiting ? 1.055f : 1.025f;
            x = std::tanh (x * drive) / std::tanh (drive);
            buffer.getWritePointer (ch)[i] = x;
        }
    }

    gainReductionDb.store (grSmooth);
}


juce::AudioProcessorEditor* PepitoLA2AAudioProcessor::createEditor()
{
    return new PepitoLA2AAudioProcessorEditor (*this);
}

void PepitoLA2AAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PepitoLA2AAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PepitoLA2AAudioProcessor();
}
