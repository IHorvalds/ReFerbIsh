#include "PluginProcessor.h"
#include "PluginEditor.h"

ReferbishAudioProcessorEditor::ReferbishAudioProcessorEditor(
    ReferbishAudioProcessor& p)
    : AudioProcessorEditor(&p), _processor(p)
{
    setSize(400, 300);
}

void ReferbishAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void ReferbishAudioProcessorEditor::resized()
{
}