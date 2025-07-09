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
#include <vector>

TrackingNodeEditor::TrackingNodeEditor (GenericProcessor* parentNode)
    : VisualizerEditor (parentNode, "Tracking"),
      selectedSource (-1),
      port (DEF_PORT),
      address (DEF_ADDRESS)
{
    desiredWidth = 250;

    sourceLabel = std::make_unique<Label> ("Source Label", "Source");
    sourceLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
    sourceLabel->setBounds (35, 24, 60, 20);
    addAndMakeVisible (sourceLabel.get());

    trackingSourceSelector = std::make_unique<ComboBox> ("Tracking Sources");
    trackingSourceSelector->setBounds (35, 45, 90, 20);
    trackingSourceSelector->addListener (this);
    addAndMakeVisible (trackingSourceSelector.get());

    plusButton = std::make_unique<UtilityButton> ("+");
    plusButton->addListener (this);
    plusButton->setRadius (3.0f);
    plusButton->setBounds (130, 45, 20, 20);
    addAndMakeVisible (plusButton.get());

    minusButton = std::make_unique<UtilityButton> ("-");
    minusButton->addListener (this);
    minusButton->setRadius (3.0f);
    minusButton->setBounds (10, 45, 20, 20);
    addAndMakeVisible (minusButton.get());

    portEditor = std::make_unique<CustomTextBox> ("Port Editor", String (DEF_PORT), "0123456789", "");
    portEditor->setBounds (165, 43, 80, 18);
    portEditor->setFont (FontOptions ("CP Mono", "Plain", int (0.75 * 18)));
    portEditor->setJustificationType (Justification::centred);
    portEditor->setEditable (true);
    portEditor->addListener (this);
    portEditor->setTooltip ("Tracking source OSC port");
    addAndMakeVisible (portEditor.get());

    portLabel = std::make_unique<Label> ("Port Label", "Port");
    portLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
    portLabel->setSize (80, 18);
    portLabel->attachToComponent (portEditor.get(), false);
    addAndMakeVisible (portLabel.get());

    addressEditor = std::make_unique<CustomTextBox> ("Address Editor", DEF_ADDRESS, String(), "");
    addressEditor->setBounds (165, 93, 80, 18);
    addressEditor->setFont (FontOptions ("CP Mono", "Plain", int (0.75 * 18)));
    addressEditor->setJustificationType (Justification::centred);
    addressEditor->setEditable (true);
    addressEditor->addListener (this);
    addressEditor->setTooltip ("Tracking source OSC address");
    addAndMakeVisible (addressEditor.get());

    addressLabel = std::make_unique<Label> ("Address Label", "Address");
    addressLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
    addressLabel->setSize (80, 18);
    addressLabel->attachToComponent (addressEditor.get(), false);
    addAndMakeVisible (addressLabel.get());

    colorSelector = std::make_unique<ComboBox> ("Color Selector");
    colorSelector->setBounds (70, 93, 80, 18);
    colorSelector->addListener (this);
    auto colors = ((TrackingNode*) parentNode)->colors;
    for (int i = 0; i < colors.size(); i++)
    {
        colorSelector->addItem (colors[i], i + 1);
    }
    colorSelector->setSelectedId (1, dontSendNotification);
    colorSelector->setTooltip ("Tracking source color");
    addAndMakeVisible (colorSelector.get());

    colorLabel = std::make_unique<Label> ("Color Label", "Color");
    colorLabel->setFont (FontOptions ("Inter", "Regular", 13.0f));
    colorLabel->setSize (80, 18);
    colorLabel->attachToComponent (colorSelector.get(), false);
    addAndMakeVisible (colorLabel.get());

    addToggleParameterEditor (Parameter::PROCESSOR_SCOPE, "StimOn", 15, 75);

    for (auto ed : parameterEditors)
    {
        if (ed->getParameterName() == "StimOn")
        {
            ed->setLayout (ParameterEditor::Layout::nameOnTop);
            ed->setSize (40, 36);
            break;
        }
    }
}

Visualizer* TrackingNodeEditor::createNewCanvas()
{
    TrackingNode* processor = (TrackingNode*) getProcessor();
    return new TrackingStimulatorCanvas (processor);
}

void TrackingNodeEditor::buttonClicked (Button* btn)
{
    TrackingNode* processor = (TrackingNode*) getProcessor();

    if (btn == plusButton.get())
    {
        // add a tracking source
        int newId = 1;
        if (trackingSourceSelector->getNumItems() > 0)
        {
            newId = trackingSourceSelector->getItemId (trackingSourceSelector->getNumItems() - 1) + 1;
        }

        String txt = "Tracking source " + String (newId);

        if (processor->addSource (txt)) // check if adding the source was successfull
        {
            trackingSourceSelector->addItem (txt, newId);
            trackingSourceSelector->setSelectedId (newId, dontSendNotification);
            selectedSource = newId - 1;

            updateCustomView();

            if (canvas)
                canvas->update();
        }
    }
    else if (btn == minusButton.get())
    {
        if (selectedSource < 0)
            return; // no source selected or invalid index

        processor->removeSource (selectedSource);

        if (selectedSource >= processor->getNumSources())
            selectedSource = processor->getNumSources() - 1;

        trackingSourceSelector->clear();
        for (int i = 0; i < processor->getNumSources(); i++)
            trackingSourceSelector->addItem ("Tracking source " + String (i + 1), i + 1);

        trackingSourceSelector->setSelectedId (selectedSource + 1, dontSendNotification);

        updateCustomView();

        if (canvas)
            canvas->update();
    }
}

void TrackingNodeEditor::comboBoxChanged (ComboBox* c)
{
    if (c == trackingSourceSelector.get())
    {
        selectedSource = c->getSelectedId() - 1;
        updateCustomView();
    }
    else if (c == colorSelector.get())
    {
        if (selectedSource < 0)
            return; // no source selected

        TrackingNode* processor = (TrackingNode*) getProcessor();
        processor->setColor (selectedSource, processor->colors[c->getSelectedId() - 1]);
    }
}

void TrackingNodeEditor::labelTextChanged (Label* label)
{
    TrackingNode* processor = (TrackingNode*) getProcessor();

    if (label == portEditor.get())
    {
        int newPort = portEditor->getText().getIntValue();
        if (newPort < 1024 || newPort > 49151) // valid port range
        {
            // if the port is invalid, set it to default
            portEditor->setText (String (port), dontSendNotification);
            LOGC ("Invalid port number. Please enter a value between 1024 and 49151.");
            return;
        }
        processor->setPort (selectedSource, newPort);
        port = newPort;
    }
    else if (label == addressEditor.get())
    {
        String newAddr = addressEditor->getText();

        if (newAddr.isEmpty())
        {
            // if the address is empty, set it to default
            addressEditor->setText (address, dontSendNotification);
            LOGC ("Empty address not allowed. Please enter a valid address.");
            return;
        }
        processor->setAddress (selectedSource, newAddr);
        address = newAddr;
    }
}

int TrackingNodeEditor::getPort()
{
    return port;
}

String TrackingNodeEditor::getAddress()
{
    return address;
}

String TrackingNodeEditor::getColor()
{
    String color = colorSelector->getText();
    if (color.isEmpty())
        color = DEF_COLOR; // default color if empty

    return color;
}

void TrackingNodeEditor::saveVisualizerEditorParameters (XmlElement* xml)
{
    XmlElement* mainNode = xml->createNewChildElement ("TRACKING_SOURCES");
    mainNode->setAttribute ("selectedID", selectedSource);

    TrackingNode* processor = (TrackingNode*) getProcessor();

    for (int i = 0; i < trackingSourceSelector->getNumItems(); i++)
    {
        XmlElement* source = new XmlElement ("Source" + String (i + 1));
        source->setAttribute ("port", processor->getPort (i));
        source->setAttribute ("address", processor->getAddress (i));
        source->setAttribute ("color", processor->getColor (i));
        mainNode->addChildElement (source);
    }
}

void TrackingNodeEditor::loadVisualizerEditorParameters (XmlElement* xml)
{
    TrackingNode* processor = (TrackingNode*) getProcessor();
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

            processor->addSource (srcName,
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

void TrackingNodeEditor::updateCustomView()
{
    TrackingNode* processor = (TrackingNode*) getProcessor();

    int newPort = processor->getPort (selectedSource);

    if (newPort == 0)
        newPort = DEF_PORT;

    portEditor->setText (String (newPort), dontSendNotification);
    port = newPort;

    String newAddr = processor->getAddress (selectedSource);

    if (newAddr.isEmpty())
        newAddr = DEF_ADDRESS;

    addressEditor->setText (newAddr, dontSendNotification);
    address = newAddr;

    String color = processor->getColor (selectedSource);

    if (color.isEmpty())
        color = DEF_COLOR;

    colorSelector->setSelectedId (processor->colors.indexOf (color) + 1, dontSendNotification);
}