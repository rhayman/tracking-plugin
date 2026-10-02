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
#include "TrackingNodeEditor.h"
#include "TrackingNode.h"
#include "TrackingStimulatorCanvas.h"

TrackingNodeEditor::TrackingNodeEditor (GenericProcessor* parentNode)
    : VisualizerEditor (parentNode, "Tracking"),
      node (static_cast<TrackingNode*> (parentNode))
{
    desiredWidth = 450;
    minusButton = std::make_unique<UtilityButton> ("-");
    minusButton->addListener (this);
    minusButton->setRadius (3.0f);
    minusButton->setBounds (10, 30, 20, 20);
    addAndMakeVisible (minusButton.get());

    plusButton = std::make_unique<UtilityButton> ("+");
    plusButton->addListener (this);
    plusButton->setRadius (3.0f);
    plusButton->setBounds (150, 30, 20, 20);
    addAndMakeVisible (plusButton.get());

    trackingSourceSelector = std::make_unique<ComboBox> ("Tracking Sources");
    trackingSourceSelector->setBounds (40, 30, 100, 20);
    trackingSourceSelector->addListener (this);
    addAndMakeVisible (trackingSourceSelector.get());
    addTextBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "port", 40, 50);
    addTextBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "address", 40, 70);
    addComboBoxParameterEditor (Parameter::PROCESSOR_SCOPE, "colour", 40, 90);
    addToggleParameterEditor (Parameter::PROCESSOR_SCOPE, "Apply", 40, 110);
}

void TrackingNodeEditor::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == trackingSourceSelector.get())
    {
        selectedSource = comboBox->getSelectedId() - 1;
        updateCustomView();
    }
    else if (comboBox == colorSelector.get())
    {
        if (selectedSource < 0)
            return; // no source selected or invalid index
        String newColor = node->colors[comboBox->getSelectedId() - 1];
        node->setColor (selectedSource, newColor);
    }
}

void TrackingNodeEditor::buttonClicked (Button* button)
{
    if (button == plusButton.get())
    {
        // add a tracking source
        int newId = 1;
        if (trackingSourceSelector->getNumItems() > 0)
        {
            newId = trackingSourceSelector->getItemId (trackingSourceSelector->getNumItems() - 1) + 1;
        }

        String txt = "Tracking source " + String (newId);
        if (node->addSource (String (txt))) // check if adding the source was successfull
        {
            trackingSourceSelector->addItem (txt, newId);
            trackingSourceSelector->setSelectedId (newId, dontSendNotification);
            selectedSource = newId - 1;

            updateCustomView();

            if (canvas)
                canvas->update();
        }
    }
    else if (button == minusButton.get())
    {
        if (selectedSource < 0)
            return; // no source selected or invalid index

        node->removeSource (selectedSource);

        if (selectedSource >= node->getNumSources())
            selectedSource = node->getNumSources() - 1;

        trackingSourceSelector->clear();
        for (int i = 0; i < node->getNumSources(); i++)
            trackingSourceSelector->addItem ("Tracking source " + String (i + 1), i + 1);

        trackingSourceSelector->setSelectedId (selectedSource + 1, dontSendNotification);

        updateCustomView();

        if (canvas)
            canvas->update();
    }
}

Visualizer* TrackingNodeEditor::createNewCanvas()
{
    TrackingNode* node = (TrackingNode*) getProcessor();

    TrackingStimulatorCanvas* nodeCanvas = new TrackingStimulatorCanvas (node);

    node->canvas = nodeCanvas;

    return nodeCanvas;
}

void TrackingNodeEditor::saveVisualizerEditorParameters (XmlElement* xml)
{
    XmlElement* mainNode = xml->createNewChildElement ("TRACKING_SOURCES");
    mainNode->setAttribute ("selectedID", selectedSource);

    for (int i = 0; i < trackingSourceSelector->getNumItems(); i++)
    {
        XmlElement* source = new XmlElement ("Source" + String (i + 1));
        source->setAttribute ("port", node->getPort (i));
        source->setAttribute ("address", node->getAddress (i));
        source->setAttribute ("color", node->getColor (i));
        mainNode->addChildElement (source);
    }
}

void TrackingNodeEditor::loadVisualizerEditorParameters (XmlElement* xml)
{
    auto* mainNode = xml->getChildByName ("TRACKING_SOURCES");

    if (mainNode != nullptr)
    {
        for (auto* source : mainNode->getChildIterator())
        {
            // add a tracking source
            int newId = 1;
            if (trackingSourceSelector->getNumItems() > 0)
            {
                newId = trackingSourceSelector->getItemId (trackingSourceSelector->getNumItems() - 1) + 1;
            }

            String srcName = "Tracking source " + String (newId);

            node->addSource (srcName,
                             source->getIntAttribute ("port"),
                             source->getStringAttribute ("address"),
                             source->getStringAttribute ("color"));

            trackingSourceSelector->addItem (srcName, newId);
        }

        selectedSource = mainNode->getIntAttribute ("selectedID");
        trackingSourceSelector->setSelectedId (selectedSource + 1, dontSendNotification);

        updateCustomView();

        if (canvas)
            canvas->update();
    }
}
int TrackingNodeEditor::getPort() const
{
    Parameter* portParam = getProcessor()->getParameter ("port");
    return (int) portParam->getValue();
}
String TrackingNodeEditor::getAddress() const
{
    Parameter* addressParam = getProcessor()->getParameter ("address");
    return (String) addressParam->getValue();
}
String TrackingNodeEditor::getColor() const
{
    Parameter* colorParam = getProcessor()->getParameter ("colour");
    return (String) colorParam->getValue();
}

// void TrackingNodeEditor::updateCustomView()
// {
// int newPort = node->getPort (selectedSource);
//
// if (newPort == 0)
//     newPort = DEF_PORT;
//
// portEditor->setText (String (newPort), dontSendNotification);
// port = newPort;
//
// String newAddr = node->getAddress (selectedSource);
//
// if (newAddr.isEmpty())
//     newAddr = DEF_ADDRESS;
//
// addressEditor->setText (newAddr, dontSendNotification);
// address = newAddr;
//
// String color = node->getColor (selectedSource);
//
// if (color.isEmpty())
//     color = DEF_COLOR;
//
// colorSelector->setSelectedId (node->colors.indexOf (color) + 1, dontSendNotification);
// }
