#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
    // Coordinates use the exact dimensions of the supplied artwork.
    constexpr float designWidth  = 1312.0f;
    constexpr float designHeight = 1199.0f;
    constexpr float designAspect = designWidth / designHeight;

    void makeInvisibleSlider (juce::Slider& slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 0.75f,
                                    juce::MathConstants<float>::pi * 2.25f, true);
        slider.setColour (juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::thumbColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::trackColourId, juce::Colours::transparentBlack);
        slider.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        slider.setDoubleClickReturnValue (true, 5.0);
    }
}

PepitoLA2AAudioProcessorEditor::PepitoLA2AAudioProcessorEditor (PepitoLA2AAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      background (juce::ImageCache::getFromMemory (BinaryData::PepitoLA2A_png,
                                                    BinaryData::PepitoLA2A_pngSize))
{
    makeInvisibleSlider (peakSlider);
    makeInvisibleSlider (gainSlider);
    addAndMakeVisible (peakSlider);
    addAndMakeVisible (gainSlider);

    limitButton.setClickingTogglesState (true);
    limitButton.setAlpha (0.001f); // Invisible click target; the lamp is painted by this editor.
    limitButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    limitButton.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    limitButton.setColour (juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
    limitButton.setColour (juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
    limitButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (limitButton);

    peakAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "peakReduction", peakSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.parameters, "gain", gainSlider);
    limitAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor.parameters, "limit", limitButton);

    setResizable (true, true);
    setResizeLimits (780, 713, 1574, 1440);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (designAspect);
    setSize (1050, 960);
    startTimerHz (30);
}

void PepitoLA2AAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff171313));
    if (background.isValid())
        g.drawImage (background, getLocalBounds().toFloat());

    const float sx = getWidth() / designWidth;
    const float sy = getHeight() / designHeight;

    // One clean metallic index per knob. The static artwork marks were removed
    // from the embedded background so there is never a doubled pointer.
    drawKnobPointer (g, { 245.0f * sx, 930.0f * sy }, peakSlider.getValue() / 10.0f, 48.0f * sx);
    drawKnobPointer (g, { 1035.0f * sx, 930.0f * sy }, gainSlider.getValue() / 10.0f, 48.0f * sx);

    drawMeterNeedle (g);
    drawLimitLamp (g);
}

void PepitoLA2AAudioProcessorEditor::drawKnobPointer (juce::Graphics& g,
                                                       juce::Point<float> centre,
                                                       float value, float radius)
{
    const float angle = juce::MathConstants<float>::pi
                      * (0.75f + 1.5f * juce::jlimit (0.0f, 1.0f, value));
    const juce::Point<float> tip (centre.x + std::cos (angle) * radius,
                                  centre.y + std::sin (angle) * radius);

    // Single narrow index line, with a small polished centre cap.
    g.setColour (juce::Colour (0xfff4e7d5).withAlpha (0.98f));
    g.drawLine (centre.x, centre.y, tip.x, tip.y, 3.0f * (getWidth() / designWidth));
    g.setColour (juce::Colour (0xff5d5148));
    g.fillEllipse (centre.x - 3.2f, centre.y - 3.2f, 6.4f, 6.4f);
}

void PepitoLA2AAudioProcessorEditor::drawMeterNeedle (juce::Graphics& g)
{
    const float sx = getWidth() / designWidth;
    const float sy = getHeight() / designHeight;
    const float gr = juce::jlimit (0.0f, 10.0f, processor.getGainReductionDb());

    // Pivot and sweep follow the printed LA-2A-style 0–10 gain-reduction scale.
    const juce::Point<float> pivot (656.0f * sx, 843.0f * sy);
    const float radius = 157.0f * sx;
    const float startAngle = juce::degreesToRadians (-158.0f);
    const float endAngle   = juce::degreesToRadians (-22.0f);
    const float angle = startAngle + (endAngle - startAngle) * (gr / 10.0f);
    const juce::Point<float> tip (pivot.x + std::cos (angle) * radius,
                                  pivot.y + std::sin (angle) * radius);

    // Exactly one needle stroke: no drop shadow and no second highlight line.
    g.setColour (juce::Colour (0xff29231e));
    g.drawLine (pivot.x, pivot.y, tip.x, tip.y, 2.6f * sx);
    g.setColour (juce::Colour (0xffd8c6a8));
    g.fillEllipse (pivot.x - 4.0f * sx, pivot.y - 4.0f * sy,
                   8.0f * sx, 8.0f * sy);
}

void PepitoLA2AAudioProcessorEditor::drawLimitLamp (juce::Graphics& g)
{
    if (limitButton.getToggleState())
        return; // The original artwork's warm yellow/orange lamp is the ON state.

    const float sx = getWidth() / designWidth;
    const float sy = getHeight() / designHeight;

    // OFF state: mute the printed amber pilot light and LIMIT button to neutral grey.
    g.setColour (juce::Colour (0xff55585b).withAlpha (0.88f));
    g.fillEllipse (juce::Rectangle<float> (619.0f * sx, 908.0f * sy, 42.0f * sx, 42.0f * sy));
    g.setColour (juce::Colour (0xff77797b).withAlpha (0.82f));
    g.fillRoundedRectangle (juce::Rectangle<float> (581.0f * sx, 968.0f * sy,
                                                    150.0f * sx, 57.0f * sy), 7.0f * sx);
    g.setColour (juce::Colour (0xffd0d0ce).withAlpha (0.90f));
    g.setFont (juce::Font (juce::FontOptions (31.0f * sx).withStyle ("Bold")));
    g.drawFittedText ("LIMIT", juce::Rectangle<int> (juce::roundToInt (590.0f * sx),
                                                      juce::roundToInt (975.0f * sy),
                                                      juce::roundToInt (132.0f * sx),
                                                      juce::roundToInt (43.0f * sy)),
                      juce::Justification::centred, 1);
}

void PepitoLA2AAudioProcessorEditor::resized()
{
    const float sx = getWidth() / designWidth;
    const float sy = getHeight() / designHeight;

    peakSlider.setBounds (juce::roundToInt (135.0f * sx), juce::roundToInt (820.0f * sy),
                          juce::roundToInt (220.0f * sx), juce::roundToInt (220.0f * sy));
    gainSlider.setBounds (juce::roundToInt (925.0f * sx), juce::roundToInt (820.0f * sy),
                          juce::roundToInt (220.0f * sx), juce::roundToInt (220.0f * sy));
    limitButton.setBounds (juce::roundToInt (548.0f * sx), juce::roundToInt (944.0f * sy),
                           juce::roundToInt (190.0f * sx), juce::roundToInt (110.0f * sy));
}

void PepitoLA2AAudioProcessorEditor::timerCallback()
{
    repaint();
}
