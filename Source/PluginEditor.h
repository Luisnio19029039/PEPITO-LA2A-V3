#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class PepitoLA2AAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit PepitoLA2AAudioProcessorEditor (PepitoLA2AAudioProcessor&);
    ~PepitoLA2AAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawKnobPointer (juce::Graphics&, juce::Point<float>, float, float);
    void drawMeterNeedle (juce::Graphics&);
    void drawLimitLamp (juce::Graphics&);

    PepitoLA2AAudioProcessor& processor;
    juce::Image background;

    juce::Slider peakSlider;
    juce::Slider gainSlider;
    juce::TextButton limitButton;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> peakAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limitAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PepitoLA2AAudioProcessorEditor)
};
