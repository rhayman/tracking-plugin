/*
------------------------------------------------------------------

This file is part of the Open Ephys GUI
Copyright (C) 2022 Open Ephys

------------------------------------------------------------------

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <vector>
#include "TrackingNodeEditor.h"
#include "TrackingNode.h"
#include "TrackingStimulatorCanvas.h"

TrackingNodeEditor::TrackingNodeEditor(GenericProcessor *parentNode)
    : VisualizerEditor(parentNode, "Tracking"),
      selectedSource(-1)
{
    desiredWidth = 250;

    sourceLabel = std::make_unique<Label>("Source Label", "SOURCE");
    sourceLabel->setFont(Font("Silkscreen", "Bold", 12.0f));
    sourceLabel->setColour(Label::textColourId, Colours::darkgrey);
    sourceLabel->setBounds(35, 24, 60, 20);
    addAndMakeVisible(sourceLabel.get());

    trackingSourceSelector = std::make_unique<ComboBox>("Tracking Sources");
    trackingSourceSelector->setBounds(35, 45, 90, 20);
    trackingSourceSelector->addListener(this);
    addAndMakeVisible(trackingSourceSelector.get());

    plusButton = std::make_unique<UtilityButton>("+", titleFont);
    plusButton->addListener(this);
    plusButton->setRadius(3.0f);
    plusButton->setBounds(130, 45, 20, 20);
    addAndMakeVisible(plusButton.get());

    minusButton = std::make_unique<UtilityButton>("-", titleFont);
    minusButton->addListener(this);
    minusButton->setRadius(3.0f);
    minusButton->setBounds(10, 45, 20, 20);
    addAndMakeVisible(minusButton.get());

    addTextBoxParameterEditor("Address", 165, 75);
    addTextBoxParameterEditor("Port", 165, 25);
    addComboBoxParameterEditor("Color", 70, 75);

     // Stimulate (toggle)
    stimLabel = std::make_unique<Label>("Stim Label", "STIM");
    stimLabel->setFont(Font("Silkscreen", "Bold", 12.0f));
    stimLabel->setColour(Label::textColourId, Colours::darkgrey);
    stimLabel->setBounds(15, 75, 40, 20);
    addAndMakeVisible(stimLabel.get());

    stimulateButton = std::make_unique<TextButton>("Stimulate Button");
    stimulateButton->setBounds(15, 95, 40, 20);
    stimulateButton->addListener(this);
    stimulateButton->setClickingTogglesState(true); // makes the button toggle its state when clicked
    stimulateButton->setButtonText("OFF");
    stimulateButton->setColour(TextButton::buttonOnColourId, Colours::yellow);
    // stimulateButton->setColour(TextButton::buttonColourId, Colours::grey);
    stimulateButton->setToggleState(false, dontSendNotification);
    addAndMakeVisible(stimulateButton.get()); // makes the button a child component of the editor and makes it visible
}

Visualizer* TrackingNodeEditor::createNewCanvas()
{
    TrackingNode* processor = (TrackingNode*) getProcessor();
    return new TrackingStimulatorCanvas(processor);
}

void TrackingNodeEditor::buttonClicked(Button *btn)
{
    TrackingNode *processor = (TrackingNode *)getProcessor();

    if (btn == plusButton.get())
    {
        // add a tracking source
        int newId = 1;
        if (trackingSourceSelector->getNumItems() > 0)
        {
            newId = trackingSourceSelector->getItemId(trackingSourceSelector->getNumItems() - 1) + 1;
        }
        
        String txt = "Tracking source " + String(newId);
        
        processor->addSource(txt);

        trackingSourceSelector->addItem(txt, newId);
        trackingSourceSelector->setSelectedId(newId, dontSendNotification);
        selectedSource = newId - 1;

        updateCustomView();

        if(canvas)
            canvas->update();
    }
    else if (btn == minusButton.get())
    {
        processor->removeSource(selectedSource);

        if (selectedSource >= processor->getNumSources())
            selectedSource = processor->getNumSources() - 1;
        
        trackingSourceSelector->clear();
        for(int i = 0; i < processor->getNumSources(); i++)
            trackingSourceSelector->addItem("Tracking source " + String(i+1), i+1);
        
        updateCustomView();

        if(canvas)
            canvas->update();

    }
    else if (btn == stimulateButton.get())
    {
        if (stimulateButton->getToggleState()==true)
        {
            processor->startStimulation();
            stimulateButton->setButtonText(String("ON"));
        }
        else
        {
            processor->stopStimulation();
            stimulateButton->setButtonText(String("OFF"));
        }
    }
}

void TrackingNodeEditor::comboBoxChanged(ComboBox* c)
{
    if (c == trackingSourceSelector.get())
    {
        selectedSource = c->getSelectedId() - 1;
        updateCustomView();
    }
}

void TrackingNodeEditor::updateCustomView()
{
    TrackingNode *processor = (TrackingNode *)getProcessor();

    auto portParam = processor->getParameter("Port");
    int port = processor->getPort(selectedSource);

    if(port == 0)
        port = DEF_PORT;

    portParam->currentValue = port;

    auto addressParam = processor->getParameter("Address");
    String addr = processor->getAddress(selectedSource);

    if(addr.isEmpty())
        addr = DEF_ADDRESS;

    addressParam->currentValue = addr;

    auto colorParam = processor->getParameter("Color");
    String color = processor->getColor(selectedSource);
    
    if(color.isEmpty())
        color = DEF_COLOR;

    colorParam->currentValue = colors.indexOf(color);

    updateView();
}