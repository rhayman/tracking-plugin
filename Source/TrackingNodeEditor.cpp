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
#include "TrackingVisualizerCanvas.h"

TrackingNodeEditor::TrackingNodeEditor(GenericProcessor *parentNode)
    : VisualizerEditor(parentNode, "Tracking"),
      selectedSource(-1)
{
    desiredWidth = 250;

    sourceLabel = std::make_unique<Label>("Source Label", "SOURCE");
    sourceLabel->setFont(Font("Silkscreen", "Bold", 12.0f));
    sourceLabel->setColour(Label::textColourId, Colours::darkgrey);
    sourceLabel->setBounds(65, 24, 60, 20);
    addAndMakeVisible(sourceLabel.get());

    trackingSourceSelector = std::make_unique<ComboBox>("Tracking Sources");
    trackingSourceSelector->setBounds(55, 45, 90, 20);
    trackingSourceSelector->addListener(this);
    addAndMakeVisible(trackingSourceSelector.get());

    plusButton = std::make_unique<UtilityButton>("+", titleFont);
    plusButton->addListener(this);
    plusButton->setRadius(3.0f);
    plusButton->setBounds(30, 45, 20, 20);
    addAndMakeVisible(plusButton.get());

    minusButton = std::make_unique<UtilityButton>("-", titleFont);
    minusButton->addListener(this);
    minusButton->setRadius(3.0f);
    minusButton->setBounds(5, 45, 20, 20);
    addAndMakeVisible(minusButton.get());

    addTextBoxParameterEditor("Address", 160, 75);
    addTextBoxParameterEditor("Port", 160, 25);
    addComboBoxParameterEditor("Color", 30, 75);
}

Visualizer* TrackingNodeEditor::createNewCanvas()
{
    TrackingNode* processor = (TrackingNode*) getProcessor();
    return new TrackingVisualizerCanvas(processor);
}

void TrackingNodeEditor::buttonClicked(Button *btn)
{
    if (btn == plusButton.get())
    {
        // add a tracking source
        TrackingNode *processor = (TrackingNode *)getProcessor();
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

    }
    if (btn == minusButton.get())
    {
        TrackingNode *processor = (TrackingNode *)getProcessor();

        processor->removeSource(selectedSource);

        if (selectedSource >= processor->getNumSources())
            selectedSource = processor->getNumSources() - 1;
        
        trackingSourceSelector->clear();
        for(int i = 0; i < processor->getNumSources(); i++)
            trackingSourceSelector->addItem("Tracking source " + String(i+1), i+1);

    }

    updateCustomView();

    if(canvas)
        canvas->update();
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