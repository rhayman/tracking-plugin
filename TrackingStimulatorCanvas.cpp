/*
    ------------------------------------------------------------------

    This file is part of the Tracking plugin for the Open Ephys GUI
    Written by:

    Alessio Buccino     alessiob@ifi.uio.no
    Mikkel Lepperod
    Svenn-Arne Dragly

    Center for Integrated Neuroplasticity CINPLA
    Department of Biosciences
    University of Oslo
    Norway

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

#include "TrackingStimulatorCanvas.h"
#include "TrackingNode.h"

TrackingStimulatorCanvas::TrackingStimulatorCanvas (GenericProcessor* sourceNode, TrackingNode* thread)
    : Visualizer (sourceNode), processor (thread), m_width (1.0), m_height (1.0), m_updateCircle (true), m_onoff (false), m_isDeleting (true), settingsWidth (250), settingsHeight (600), selectedSource (-1), outputChan (0), sectionLabelFont ("Inter", "Semi Bold", 18.0f)
{
    // Setup buttons
    initButtons();
    // Setup Labels
    initLabels();

    addKeyListener (this);
    setWantsKeyboardFocus (true);

    displayAxes = std::make_unique<DisplayAxes> (thread, this);
    addAndMakeVisible (displayAxes.get());

    update();
}

TrackingStimulatorCanvas::~TrackingStimulatorCanvas()
{
    removeKeyListener (this);
}

void TrackingStimulatorCanvas::initButtons()
{
    FontOptions buttonFont = FontOptions (14.0f);

    clearButton = std::make_unique<UtilityButton> ("Clear plot");
    clearButton->setFont (buttonFont);
    clearButton->setRadius (3.0f);
    clearButton->onClick = [this]
    { clear(); };
    // clearButton->addListener(this);
    addAndMakeVisible (clearButton.get());

    simTrajectoryButton = std::make_unique<UtilityButton> ("Simulate");
    simTrajectoryButton->setFont (buttonFont);
    simTrajectoryButton->setRadius (3.0f);
    simTrajectoryButton->addListener (this);
    simTrajectoryButton->setClickingTogglesState (true);
    addAndMakeVisible (simTrajectoryButton.get());

    newButton = std::make_unique<UtilityButton> ("New");
    newButton->setFont (buttonFont);
    newButton->setRadius (3.0f);
    newButton->addListener (this);
    addAndMakeVisible (newButton.get());

    editButton = std::make_unique<UtilityButton> ("Edit");
    editButton->setFont (buttonFont);
    editButton->setRadius (3.0f);
    editButton->addListener (this);
    addAndMakeVisible (editButton.get());

    delButton = std::make_unique<UtilityButton> ("Delete");
    delButton->setFont (buttonFont);
    delButton->setRadius (3.0f);
    delButton->addListener (this);
    addAndMakeVisible (delButton.get());

    onButton = std::make_unique<TextButton> ("Disabled");
    onButton->setClickingTogglesState (true);
    onButton->setToggleState (false, dontSendNotification);
    onButton->setColour (TextButton::buttonOnColourId, Colours::green);
    onButton->setColour (TextButton::buttonColourId, Colours::darkred);
    onButton->setColour (TextButton::textColourOnId, Colours::white);
    onButton->setColour (TextButton::textColourOffId, Colours::lightgrey);
    onButton->addListener (this);
    addAndMakeVisible (onButton.get());

    availableSources = std::make_unique<ComboBox> ("Tracking Sources");
    availableSources->setEditableText (false);
    availableSources->setJustificationType (Justification::centredLeft);
    availableSources->addListener (this);
    availableSources->setSelectedId (0);
    addAndMakeVisible (availableSources.get());

    outputChans = std::make_unique<ComboBox> ("Output Channels");
    outputChans->setEditableText (false);
    outputChans->setJustificationType (Justification::centredLeft);
    outputChans->addListener (this);
    outputChans->setSelectedId (0);

    for (int i = 1; i < 9; i++)
        outputChans->addItem (String (i), i);

    outputChans->setSelectedId (processor->getOutputChan() + 1, dontSendNotification);
    addAndMakeVisible (outputChans.get());

    // Create invisible circle toggle button
    for (int i = 0; i < MAX_CIRCLES; i++)
    {
        circlesButton[i] = std::make_unique<UtilityButton> (String (i + 1));
        circlesButton[i]->setFont (buttonFont);
        circlesButton[i]->setRadioGroupId (101);
        circlesButton[i]->setRadius (5.0f);
        circlesButton[i]->addListener (this);
        circlesButton[i]->setClickingTogglesState (true);
        addAndMakeVisible (circlesButton[i].get());
    }

    uniformButton = std::make_unique<UtilityButton> ("Uniform");
    uniformButton->setFont (buttonFont);
    uniformButton->setRadioGroupId (201);
    uniformButton->setRadius (3.0f);
    uniformButton->addListener (this);
    uniformButton->setClickingTogglesState (true);
    addAndMakeVisible (uniformButton.get());

    gaussianButton = std::make_unique<UtilityButton> ("Gaussian");
    gaussianButton->setFont (buttonFont);
    gaussianButton->setRadioGroupId (201);
    gaussianButton->setRadius (3.0f);
    gaussianButton->addListener (this);
    gaussianButton->setClickingTogglesState (true);
    addAndMakeVisible (gaussianButton.get());

    ttlButton = std::make_unique<UtilityButton> ("Single");
    ttlButton->setFont (buttonFont);
    ttlButton->setRadioGroupId (201);
    ttlButton->setRadius (3.0f);
    ttlButton->addListener (this);
    ttlButton->setClickingTogglesState (true);
    addAndMakeVisible (ttlButton.get());

    // Update button toggle state with current chan1 parameters
    if (processor->getStimMode() == uniform)
        uniformButton->triggerClick();
    else if (processor->getStimMode() == gauss)
        gaussianButton->triggerClick();
    else if (processor->getStimMode() == ttl)
        ttlButton->triggerClick();
}

void TrackingStimulatorCanvas::initLabels()
{
    FontOptions paramFont = FontOptions ("Inter", "Regular", 14.0f);
    // Static Labels
    sourcesLabel = std::make_unique<Label> ("s_sources", "Input Source");
    sourcesLabel->setFont (sectionLabelFont);
    addAndMakeVisible (sourcesLabel.get());

    outputLabel = std::make_unique<Label> ("s_output", "Output Line");
    outputLabel->setFont (sectionLabelFont);
    addAndMakeVisible (outputLabel.get());

    circlesLabel = std::make_unique<Label> ("s_circles", "Circles");
    circlesLabel->setFont (sectionLabelFont);
    addAndMakeVisible (circlesLabel.get());

    paramLabel = std::make_unique<Label> ("s_param", "Trigger parameters");
    paramLabel->setFont (sectionLabelFont);
    addAndMakeVisible (paramLabel.get());

    fmaxLabel = std::make_unique<Label> ("s_fmax", "FMax [Hz]:");
    fmaxLabel->setFont (paramFont);
    addAndMakeVisible (fmaxLabel.get());

    sdevLabel = std::make_unique<Label> ("s_sdev", "SD [%]:");
    sdevLabel->setFont (paramFont);
    addAndMakeVisible (sdevLabel.get());

    durationLabel = std::make_unique<Label> ("s_dur", "Duration [ms]:");
    durationLabel->setFont (paramFont);
    addAndMakeVisible (durationLabel.get());

    fmaxEditLabel = std::make_unique<CustomTextBox> ("fmax", String (processor->getStimFreq()), "0123456789.");
    fmaxEditLabel->setEditable (true);
    fmaxEditLabel->addListener (this);
    addAndMakeVisible (fmaxEditLabel.get());

    sdevEditLabel = std::make_unique<CustomTextBox> ("sdev", String (processor->getStimSD()), "0123456789.");
    sdevEditLabel->setEditable (true);
    sdevEditLabel->addListener (this);
    addAndMakeVisible (sdevEditLabel.get());

    durationEditLabel = std::make_unique<CustomTextBox> ("sdur", String (processor->getTTLDuration()), "0123456789");
    durationEditLabel->setEditable (true);
    durationEditLabel->addListener (this);
    addAndMakeVisible (durationEditLabel.get());

    if (processor->getStimMode() == gauss)
    {
        sdevLabel->setVisible (true);
        sdevEditLabel->setVisible (true);
    }
    else
    {
        sdevLabel->setVisible (false);
        sdevEditLabel->setVisible (false);
        fmaxLabel->setVisible (false);
        fmaxEditLabel->setVisible (false);
    }
}

float TrackingStimulatorCanvas::my_round (float x)
{
    return x < 0.0 ? ceil (x - 0.5) : floor (x + 0.5);
}

bool TrackingStimulatorCanvas::areThereCicles()
{
    if (processor->getCircles().size() > 0)
        return true;
    else
        return false;
}

void TrackingStimulatorCanvas::paint (Graphics& g)
{
    g.setColour (findColour (ThemeColours::componentParentBackground)); // background color
    g.fillRect (0, 0, getWidth(), getHeight());

    g.setColour (findColour (ThemeColours::componentBackground)); //settings menu background color
    g.fillRoundedRectangle (getWidth() - settingsWidth - 10, 10, settingsWidth, settingsHeight - 20, 7.0f);

    displayAxes->repaint();
    // i was calling refresh() here but it was causing a crash
    // such that you couldnt close / interact with the GUI
    //
}

void TrackingStimulatorCanvas::resized()
{
    if (0.22 * getWidth() < 200)
        settingsWidth = 200;
    else if (0.22 * getWidth() > 300)
        settingsWidth = 300;
    else
        settingsWidth = 0.22 * getWidth();

    if (getHeight() < 550)
        settingsHeight = 550;
    else if (getHeight() > 750)
        settingsHeight = 750;
    else
        settingsHeight = getHeight();

    float plot_height = 0.98 * getHeight();
    float plot_width = 0.75 * getWidth();
    float plot_bottom_left_x = 0.01 * getWidth();
    float plot_bottom_left_y = 0.01 * getHeight();

    float left_limit = getWidth() - settingsWidth - 20;

    // set aspect ratio to cam size
    float aC = m_width / m_height;
    float aS = plot_width / plot_height;
    int camHeight = (aS > aC) ? plot_height : plot_height * (aS / aC);
    int camWidth = (aS < aC) ? plot_width : plot_width * (aC / aS);

    if (camWidth > left_limit)
    {
        camWidth = left_limit;
        camHeight = camWidth / aC;
    }

    //    displayAxes->setBounds(int(0.01*getHeight()), int(0.01*getHeight()), int(0.98*getHeight()), int(0.98*getHeight()));
    displayAxes->setBounds (int (plot_bottom_left_x), int (plot_bottom_left_y), int (camWidth), int (camHeight));

    sourcesLabel->setBounds (getWidth() - settingsWidth, 25, settingsWidth - 20, 25);
    availableSources->setBounds (getWidth() - settingsWidth, 55, settingsWidth - 20, 25);

    outputLabel->setBounds (getWidth() - settingsWidth, 0.16 * settingsHeight, settingsWidth, 25);
    outputChans->setBounds (getWidth() - settingsWidth, 0.21 * settingsHeight, settingsWidth - 20, 25);

    circlesLabel->setBounds (getWidth() - settingsWidth, 0.3 * settingsHeight, settingsWidth - 20, 25);
    newButton->setBounds (getWidth() - settingsWidth, 0.35 * settingsHeight, settingsWidth / 3 - 5, 25);
    editButton->setBounds (getWidth() - (2 * settingsWidth / 3) - 5, 0.35 * settingsHeight, settingsWidth / 3 - 5, 25);
    delButton->setBounds (getWidth() - (settingsWidth / 3) - 10, 0.35 * settingsHeight, settingsWidth / 3 - 5, 25);

    onButton->setBounds (getWidth() - (3 * settingsWidth / 4), 0.5 * settingsHeight, settingsWidth / 2 - 30, 25);

    for (int i = 0; i < MAX_CIRCLES; i++)
    {
        circlesButton[i]->setBounds (getWidth() - settingsWidth + i * (settingsWidth - 20) / MAX_CIRCLES, 0.42 * settingsHeight, (settingsWidth - 20) / MAX_CIRCLES, 25);

        if (i < processor->getCircles().size())
            circlesButton[i]->setVisible (true);
        else
            circlesButton[i]->setVisible (false);
    }

    paramLabel->setBounds (getWidth() - settingsWidth, 0.6 * settingsHeight, settingsWidth - 20, 25);
    uniformButton->setBounds (getWidth() - settingsWidth, 0.65 * settingsHeight, settingsWidth / 3 - 5, 25);
    gaussianButton->setBounds (getWidth() - (2 * settingsWidth / 3) - 5, 0.65 * settingsHeight, settingsWidth / 3 - 5, 25);
    ttlButton->setBounds (getWidth() - (settingsWidth / 3) - 10, 0.65 * settingsHeight, settingsWidth / 3 - 5, 25);

    durationLabel->setBounds (getWidth() - settingsWidth, 0.71 * settingsHeight, settingsWidth / 2 - 20, 25);
    durationEditLabel->setBounds (getWidth() - (settingsWidth / 2), 0.71 * settingsHeight, settingsWidth / 2 - 20, 25);

    fmaxLabel->setBounds (getWidth() - settingsWidth, 0.765 * settingsHeight, settingsWidth / 2 - 20, 25);
    fmaxEditLabel->setBounds (getWidth() - (settingsWidth / 2), 0.765 * settingsHeight, settingsWidth / 2 - 20, 25);

    sdevLabel->setBounds (getWidth() - settingsWidth, 0.82 * settingsHeight, settingsWidth / 2 - 20, 25);
    sdevEditLabel->setBounds (getWidth() - (settingsWidth / 2), 0.82 * settingsHeight, settingsWidth / 2 - 20, 25);

    simTrajectoryButton->setBounds (getWidth() - settingsWidth, settingsHeight - 50, settingsWidth / 2 - 20, 25);
    clearButton->setBounds (getWidth() - (settingsWidth / 2), settingsHeight - 50, settingsWidth / 2 - 20, 25);

    refresh();
}

void TrackingStimulatorCanvas::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == availableSources.get())
    {
        if (comboBox->getSelectedId() > 1)
        {
            int index = comboBox->getSelectedId() - 2;
            selectedSource = index;
        }
        else
            selectedSource = -1;
        processor->setSelectedStimSource (selectedSource);
    }
    else if (comboBox == outputChans.get())
    {
        if (comboBox->getSelectedId() > 0)
        {
            int index = comboBox->getSelectedId() - 1;
            outputChan = index;
        }
        else
            outputChan = -1;
        processor->setOutputChan (outputChan);
    }
}

bool TrackingStimulatorCanvas::keyPressed (const KeyPress& key, Component* originatingComponent)
{
    //copy/paste/delete of circles
    if (key.getKeyCode() == key.deleteKey)
    {
        if (areThereCicles())
        {
            delButton->triggerClick();
            return true;
        }
    }
    else if (key == KeyPress ('c', ModifierKeys::ctrlModifier, 0))
    {
        if (areThereCicles())
        {
            displayAxes->copy();
            return true;
        }
    }
    else if (key == KeyPress ('v', ModifierKeys::ctrlModifier, 0))
    {
        if (areThereCicles())
        {
            displayAxes->paste();
            return true;
        }
    }
    return false;
}

void TrackingStimulatorCanvas::buttonClicked (Button* button)
{
    if (button == clearButton.get())
    {
        clear();
    }
    else if (button == simTrajectoryButton.get())
    {
        if (simTrajectoryButton->getToggleState() == true)
        {
            processor->setSimulateTrajectory (true);
            m_width = 1;
            m_height = 1;
        }
        else
            processor->setSimulateTrajectory (false);
    }
    else if (button == newButton.get())
    {
        float cx, cy, crad;

        if (processor->getSelectedCircle() >= 0)
        {
            cx = processor->getCircles()[processor->getSelectedCircle()].getX() * 100;
            cy = processor->getCircles()[processor->getSelectedCircle()].getY() * 100;
            crad = processor->getCircles()[processor->getSelectedCircle()].getRad() * 100;
        }
        else
        {
            cx = 25.0f;
            cy = 25.0f;
            crad = 10.0f;
        }

        CircleEditor* newCircleEditor = new CircleEditor (this, false, cx, cy, crad);

        CallOutBox& myBox = CallOutBox::launchAsynchronously (
            std::unique_ptr<Component> (newCircleEditor),
            button->getScreenBounds(),
            nullptr);

        myBox.setDismissalMouseClicksAreAlwaysConsumed (true);
    }
    else if (button == editButton.get())
    {
        if (processor->getSelectedCircle() >= 0)
        {
            float cx = processor->getCircles()[processor->getSelectedCircle()].getX() * 100;
            float cy = processor->getCircles()[processor->getSelectedCircle()].getY() * 100;
            float crad = processor->getCircles()[processor->getSelectedCircle()].getRad() * 100;

            CircleEditor* newCircleEditor = new CircleEditor (this, true, cx, cy, crad);

            CallOutBox& myBox = CallOutBox::launchAsynchronously (
                std::unique_ptr<Component> (newCircleEditor),
                button->getScreenBounds(),
                nullptr);

            myBox.setDismissalMouseClicksAreAlwaysConsumed (true);
        }
        else
        {
            Label* warningLabel = new Label ("warning", "Please select a circle first!");
            warningLabel->setSize (130, 60);
            warningLabel->setJustificationType (Justification::centred);

            CallOutBox& myBox = CallOutBox::launchAsynchronously (
                std::unique_ptr<Component> (warningLabel),
                button->getScreenBounds(),
                nullptr);

            myBox.setDismissalMouseClicksAreAlwaysConsumed (true);
        }
    }
    else if (button == delButton.get())
    {
        if (processor->getSelectedCircle() >= 0)
        {
            m_updateCircle = true;

            processor->deleteCircle (processor->getSelectedCircle());

            // Blank labels and untoggle all circle buttons
            processor->setSelectedCircle (-1);
            m_onoff = false;

            for (int i = 0; i < MAX_CIRCLES; i++)
                circlesButton[i]->setToggleState (false, dontSendNotification);

            // make visible only the remaining labels
            for (int i = 0; i < MAX_CIRCLES; i++)
            {
                if (i < processor->getCircles().size())
                    circlesButton[i]->setVisible (true);
                else
                    circlesButton[i]->setVisible (false);
            }
        }
        else
        {
            Label* warningLabel = new Label ("warning", "Please select a circle first!");
            warningLabel->setSize (130, 60);
            warningLabel->setJustificationType (Justification::centred);

            CallOutBox& myBox = CallOutBox::launchAsynchronously (
                std::unique_ptr<Component> (warningLabel),
                button->getScreenBounds(),
                nullptr);

            myBox.setDismissalMouseClicksAreAlwaysConsumed (true);
        }
    }
    else if (button == onButton.get())
    {
        // make changes immediately if circle already exist
        if (processor->getSelectedCircle() != -1)
        {
            m_onoff = onButton->getToggleState();

            if (m_onoff)
                onButton->setButtonText ("Enabled");
            else
                onButton->setButtonText ("Disabled");

            m_current_cx = processor->getCircles()[processor->getSelectedCircle()].getX();
            m_current_cy = processor->getCircles()[processor->getSelectedCircle()].getY();
            m_current_crad = processor->getCircles()[processor->getSelectedCircle()].getRad();
            editSelectedCircle (m_current_cx, m_current_cy, m_current_crad);
        }
        else
        {
            onButton->setToggleState (false, dontSendNotification);
            onButton->setButtonText ("Disabled");
            m_onoff = false;
        }
    }
    else if (button == uniformButton.get())
    {
        if (button->getToggleState() == true)
        {
            fmaxLabel->setVisible (true);
            fmaxEditLabel->setVisible (true);
            sdevLabel->setVisible (false);
            sdevEditLabel->setVisible (false);
            processor->setStimMode (uniform);
        }
    }
    else if (button == gaussianButton.get())
    {
        if (button->getToggleState() == true)
        {
            fmaxLabel->setVisible (true);
            fmaxEditLabel->setVisible (true);
            sdevLabel->setVisible (true);
            sdevEditLabel->setVisible (true);
            processor->setStimMode (gauss);
        }
    }
    else if (button == ttlButton.get())
    {
        if (button->getToggleState() == true)
        {
            fmaxLabel->setVisible (false);
            fmaxEditLabel->setVisible (false);
            sdevLabel->setVisible (false);
            sdevEditLabel->setVisible (false);
            processor->setStimMode (ttl);
        }
    }
    else if (button->getRadioGroupId() == 101)
    {
        // check if one of circle button has been clicked
        bool someToggled = false;
        for (int i = 0; i < MAX_CIRCLES; i++)
        {
            if (button == circlesButton[i].get() && circlesButton[i]->getEnabledState())
            {
                // toggle button and untoggle all the others + update
                if (button->getToggleState() == true)
                {
                    processor->setSelectedCircle (i);
                    someToggled = true;
                    // retrieve labels and on button values
                    if (areThereCicles())
                    {
                        m_onoff = processor->getCircles()[processor->getSelectedCircle()].getOn();
                        m_current_cx = processor->getCircles()[processor->getSelectedCircle()].getX();
                        m_current_cy = processor->getCircles()[processor->getSelectedCircle()].getY();
                        m_current_crad = processor->getCircles()[processor->getSelectedCircle()].getRad();
                        onButton->setToggleState (m_onoff, sendNotification);
                    }
                }
                break;
            }
        }
        if (! someToggled)
        {
            // blank labels
            processor->setSelectedCircle (-1);
            m_onoff = false;
        }
    }
    repaint();
}

void TrackingStimulatorCanvas::uploadCircles()
{
    // circle buttons visible
    for (int i = 0; i < MAX_CIRCLES; i++)
    {
        if (i < processor->getCircles().size())
            circlesButton[i]->setVisible (true);
        else
            circlesButton[i]->setVisible (false);
    }
}

void TrackingStimulatorCanvas::labelTextChanged (Label* label)
{
    if (label == fmaxEditLabel.get())
    {
        Value val = label->getTextValue();
        if ((float (val.getValue()) >= 0 && float (val.getValue()) <= 10000))
            processor->setStimFreq (float (val.getValue()));
        else
        {
            CoreServices::sendStatusMessage ("Selected values cannot be negative!");
            label->setText ("", dontSendNotification);
        }
    }
    if (label == sdevEditLabel.get())
    {
        Value val = label->getTextValue();
        if ((float (val.getValue()) >= 0 && float (val.getValue()) <= 1))
            if (float (val.getValue()) > 0)
                processor->setStimSD (float (val.getValue()));
            else
                processor->setStimSD (1e-10);
        else
        {
            CoreServices::sendStatusMessage ("Selected values must be between 0 and 1!");
            label->setText ("", dontSendNotification);
        }
    }
    if (label == durationEditLabel.get())
    {
        Value val = label->getTextValue();
        if (int (val.getValue()) >= 0)
            processor->setTTLDuration (int (val.getValue()));
        else
        {
            CoreServices::sendStatusMessage ("Selected values cannot be negative!");
            label->setText ("", dontSendNotification);
        }
    }
}

void TrackingStimulatorCanvas::updateSettings()
{
    availableSources->clear();
    int nSources = processor->getNumSources();
    int nextItem = 2;
    availableSources->addItem ("SELECT", 1);
    for (int i = 0; i < nSources; i++)
    {
        TrackingSources& source = processor->getTrackingSource (i);
        String name = source.name;
        availableSources->addItem (name, nextItem++);
    }

    if (nSources == 0)
    {
        selectedSource = -1;
        availableSources->setSelectedId (1, dontSendNotification);
        processor->setSelectedStimSource (-1);
        return;
    }

    if (selectedSource < 0)
        selectedSource = 0;
    else if (selectedSource >= nSources)
        selectedSource = nSources - 1;

    availableSources->setSelectedId (selectedSource + 2, dontSendNotification); // first is SELECT
    processor->setSelectedStimSource (selectedSource);
}

void TrackingStimulatorCanvas::refresh()
{
    if (processor->positionIsUpdated())
    {
        for (int i = 0; i < processor->getNumSources(); i++)
        {
            auto positionData = processor->getTrackingPositions (i);

            for (int j = 0; j < positionData.size(); j++)
            {
                displayAxes->addPosition (i, positionData[j]);
            }

            // for now, just pick one w and h
            TrackingSources& source = processor->getTrackingSource (i);
            m_height = source.width;
            m_width = source.height;
        }
        processor->clearPositionUpdated();
        repaint();
    }
}

void TrackingStimulatorCanvas::beginAnimation()
{
    startCallbacks();
}

void TrackingStimulatorCanvas::endAnimation()
{
    stopCallbacks();
}

void TrackingStimulatorCanvas::saveCustomParametersToXml (XmlElement* xml)
{
    XmlElement* state = xml->createNewChildElement ("TRACKING_STIMULATOR");
    state->setAttribute ("Source", selectedSource);
    state->setAttribute ("Output", outputChan);

    // save circles
    XmlElement* circles = new XmlElement ("CIRCLES");
    for (int i = 0; i < processor->getCircles().size(); i++)
    {
        XmlElement* circ = new XmlElement (String ("Circles_") += String (i));
        auto circle = processor->getCircles()[i];
        circ->setAttribute ("id", i);
        circ->setAttribute ("xpos", circle.getX());
        circ->setAttribute ("ypos", circle.getY());
        circ->setAttribute ("rad", circle.getRad());
        circ->setAttribute ("on", circle.getOn());

        circles->addChildElement (circ);
    }
    circles->setAttribute ("selectedCircle", processor->getSelectedCircle());

    // save stimulator conf
    XmlElement* stim = new XmlElement ("STIM_PARAMETERS");

    stim->setAttribute ("duration", durationEditLabel->getText());
    stim->setAttribute ("freq", fmaxEditLabel->getText());
    stim->setAttribute ("sd", sdevEditLabel->getText());
    stim->setAttribute ("stim-mode", processor->getStimMode());

    state->addChildElement (circles);
    state->addChildElement (stim);
}

void TrackingStimulatorCanvas::loadCustomParametersFromXml (XmlElement* xml)
{
    auto* mainNode = xml->getChildByName ("TRACKING_STIMULATOR");

    if (mainNode != nullptr)
    {
        selectedSource = mainNode->getIntAttribute ("Source");
        processor->setSelectedStimSource (selectedSource);
        update();

        outputChan = mainNode->getIntAttribute ("Output");
        processor->setOutputChan (outputChan);

        for (auto* childElement : mainNode->getChildIterator())
        {
            if (childElement->hasTagName ("CIRCLES"))
            {
                for (auto* circleXml : childElement->getChildIterator())
                {
                    int id = circleXml->getIntAttribute ("id");
                    double cx = circleXml->getDoubleAttribute ("xpos");
                    double cy = circleXml->getDoubleAttribute ("ypos");
                    double crad = circleXml->getDoubleAttribute ("rad");
                    bool on = circleXml->getIntAttribute ("on");

                    processor->addCircle (StimCircle (cx, cy, crad, on));
                }

                uploadCircles();
                int selectedCircle = childElement->getIntAttribute ("selectedCircle");
                if (selectedCircle >= 0)
                    circlesButton[selectedCircle]->triggerClick();
            }
            if (childElement->hasTagName ("STIMULATION"))
            {
                durationEditLabel->setText (childElement->getStringAttribute ("duration"), sendNotification);
                fmaxEditLabel->setText (childElement->getStringAttribute ("freq"), sendNotification);
                sdevEditLabel->setText (childElement->getStringAttribute ("sd"), sendNotification);

                stim_mode currentMode = (stim_mode) childElement->getIntAttribute ("stim-mode");

                switch (currentMode)
                {
                    case stim_mode::uniform:
                        uniformButton->setToggleState (true, sendNotification);
                        break;

                    case stim_mode::gauss:
                        gaussianButton->setToggleState (true, sendNotification);
                        break;

                    case stim_mode::ttl:
                        ttlButton->setToggleState (true, sendNotification);
                        break;
                }
            }
        }
    }
}

int TrackingStimulatorCanvas::getSelectedSource() const
{
    return selectedSource;
}

void TrackingStimulatorCanvas::clear()
{
    // set all circles to off
    // processor->disableCircles();
    displayAxes->clear();
    repaint();
}

void TrackingStimulatorCanvas::createCircle (float cx, float cy, float rad)
{
    if (processor->getCircles().size() < MAX_CIRCLES)
    {
        setOnButton();
        processor->addCircle (StimCircle (cx, cy, rad, m_onoff));
        processor->setSelectedCircle (processor->getCircles().size() - 1);
        circlesButton[processor->getSelectedCircle()]->setVisible (true);

        // toggle current circle button (untoggles all the others)
        if (circlesButton[processor->getSelectedCircle()]->getToggleState() == false)
            circlesButton[processor->getSelectedCircle()]->triggerClick();

        m_isDeleting = false;
        m_updateCircle = true;
    }
    else
        CoreServices::sendStatusMessage ("Max number of circles reached!");
}

void TrackingStimulatorCanvas::selectCircle (int circleIdx)
{
    if (! circlesButton[circleIdx]->getToggleState())
        circlesButton[circleIdx]->triggerClick();
}

void TrackingStimulatorCanvas::editSelectedCircle (float cx, float cy, float rad)
{
    if (areThereCicles())
        processor->editCircle (processor->getSelectedCircle(), cx, cy, rad, m_onoff);

    m_updateCircle = true;
}

bool TrackingStimulatorCanvas::getUpdateCircle()
{
    return m_updateCircle;
}

void TrackingStimulatorCanvas::setUpdateCircle (bool onoff)
{
    m_updateCircle = onoff;
}

void TrackingStimulatorCanvas::setOnButton()
{
    m_onoff = true;
}

/** ------------- Circle Editor Component --------------- */

CircleEditor::CircleEditor (TrackingStimulatorCanvas* stimCanvas, bool isEditMode_, float cx, float cy, float cRad)
    : canvas (stimCanvas), isEditMode (isEditMode_), xVal (cx), yVal (cy), radius (cRad)
{
    FontOptions labelFont = FontOptions ("Inter", "Semi Bold", 16.0f);

    cxLabel = std::make_unique<Label> ("CX", "X [%] :");
    cxLabel->setFont (labelFont);
    cxLabel->setBounds (10, 10, 55, 30);
    addAndMakeVisible (cxLabel.get());

    cxSlider = std::make_unique<Slider> ("CXSlider");
    cxSlider->setSliderStyle (Slider::LinearHorizontal);
    cxSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 40, 25);
    cxSlider->setRange (0, 100, 1);
    cxSlider->setValue (xVal, dontSendNotification);
    cxSlider->setBounds (70, 10, 160, 30);
    addAndMakeVisible (cxSlider.get());

    cyLabel = std::make_unique<Label> ("CY", "Y [%] :");
    cyLabel->setFont (labelFont);
    cyLabel->setBounds (10, 50, 55, 30);
    addAndMakeVisible (cyLabel.get());

    cySlider = std::make_unique<Slider> ("CYSlider");
    cySlider->setSliderStyle (Slider::LinearHorizontal);
    cySlider->setTextBoxStyle (Slider::TextBoxLeft, false, 40, 25);
    cySlider->setRange (0, 100, 1);
    cySlider->setValue (yVal, dontSendNotification);
    cySlider->setBounds (70, 50, 160, 30);
    addAndMakeVisible (cySlider.get());

    cradLabel = std::make_unique<Label> ("CRAD", "Rad [%] :");
    cradLabel->setFont (labelFont);
    cradLabel->setBounds (10, 90, 55, 30);
    addAndMakeVisible (cradLabel.get());

    cradSlider = std::make_unique<Slider> ("CRadSlider");
    cradSlider->setSliderStyle (Slider::LinearHorizontal);
    cradSlider->setTextBoxStyle (Slider::TextBoxLeft, false, 40, 25);
    cradSlider->setRange (1, 100, 1);
    cradSlider->setValue (radius, dontSendNotification);
    cradSlider->setBounds (70, 90, 160, 30);
    addAndMakeVisible (cradSlider.get());

    if (isEditMode)
    {
        cxSlider->onValueChange = [this]
        { updateCircleParams(); };
        cySlider->onValueChange = [this]
        { updateCircleParams(); };
        cradSlider->onValueChange = [this]
        { updateCircleParams(); };
        setSize (240, 130);
    }
    else
    {
        createButton = std::make_unique<TextButton> ("Create");
        createButton->setButtonText ("Create");
        createButton->setBounds (85, 140, 70, 30);
        addAndMakeVisible (createButton.get());
        createButton->onClick = [this]
        { createNewCircle(); };

        setSize (240, 180);
    }
}

CircleEditor::~CircleEditor()
{
}

void CircleEditor::paint (Graphics& g)
{
    g.fillAll (findColour (ThemeColours::componentBackground));
}

void CircleEditor::updateCircleParams()
{
    xVal = cxSlider->getValue() / 100;
    yVal = cySlider->getValue() / 100;
    radius = cradSlider->getValue() / 100;

    canvas->editSelectedCircle (xVal, yVal, radius);
}

void CircleEditor::createNewCircle()
{
    xVal = cxSlider->getValue() / 100;
    yVal = cySlider->getValue() / 100;
    radius = cradSlider->getValue() / 100;

    canvas->createCircle (xVal, yVal, radius);

    findParentComponentOfClass<CallOutBox>()->exitModalState (0);
}

/** ------------- Canvas Display Axes --------------- */

DisplayAxes::DisplayAxes (TrackingNode* node, TrackingStimulatorCanvas* canvas_)
    : processor (node), canvas (canvas_), circleColour (Colour (225, 200, 30)), m_creatingNewCircle (false), m_movingCircle (false), m_mayBeMoving (false), m_resizing (false), m_firstPaint (true), m_copy (false)

{
    color_palette["red"] = Colours::red;
    color_palette["green"] = Colours::green;
    color_palette["blue"] = Colours::blue;
    color_palette["cyan"] = Colours::cyan;
    color_palette["magenta"] = Colours::magenta;
    color_palette["orange"] = Colours::orange;
    color_palette["pink"] = Colours::pink;
    color_palette["grey"] = Colours::grey;
    color_palette["violet"] = Colours::violet;
    color_palette["yellow"] = Colours::yellow;
    color_palette["white"] = Colours::white;
}

DisplayAxes::~DisplayAxes() {}

void DisplayAxes::addPosition (int index, TrackingPosition& postionData)
{
    m_positions[index].push_back (postionData);
    // LOGC ("addPosition in displayAxes");
    // LOGC ("m_positions size", m_positions[index].size());
    // this works now but the m_positions vector is just growing
}

void DisplayAxes::paint (Graphics& g)
{
    g.setColour (Colour (0, 18, 43)); //background color
    g.fillAll();

    if (canvas->getUpdateCircle())
    {
        for (int i = 0; i < processor->getCircles().size(); i++)
        {
            float cur_x, cur_y, cur_rad;
            int x_c, y_c, x, y, radx, rady;

            cur_x = processor->getCircles()[i].getX();
            cur_y = processor->getCircles()[i].getY();
            cur_rad = processor->getCircles()[i].getRad();

            x_c = int (cur_x);
            y_c = int (cur_y);

            radx = int (cur_rad);
            rady = int (cur_rad);
            // center ellipse
            x = x_c - radx;
            y = y_c - rady;

            // draw circle if it is ON
            if (processor->getCircles()[i].getOn())
            {
                if (processor->getStimMode() == uniform || processor->getStimMode() == ttl)
                    g.setColour (circleColour);
                else
                {
                    ColourGradient Cgrad = ColourGradient (Colours::darkorange, double (x_c), double (y_c), Colours::lightyellow, double (x_c + radx), double (y_c + rady), true);
                    g.setGradientFill (Cgrad);
                }
            }
            else
                g.setColour (Colours::grey.withAlpha (0.5f));

            if (i == processor->getSelectedCircle())
            {
                // if circle is being moved or changed size, don't draw static circle
                if (! (m_movingCircle || m_resizing))
                {
                    g.fillEllipse (x, y, 2 * radx, 2 * rady);
                    g.setColour (Colours::white);
                    g.drawEllipse (x, y, 2 * radx, 2 * rady, 3.0f);

                    // Display circle number
                    g.setColour (Colours::white);
                    g.setFont (18.0f);
                    g.drawRect (x_c - 10, y_c - 10, 20, 20, 2);
                    g.drawFittedText (String (i + 1), x_c - 10, y_c - 10, 20, 20, Justification::centred, 1);
                }
            }
            else
            {
                g.fillEllipse (x, y, 2 * radx, 2 * rady);

                // Display circle number
                g.setColour (Colours::white);
                g.setFont (18.0f);
                g.drawFittedText (String (i + 1), x_c - 10, y_c - 10, 20, 20, Justification::centred, 1);
            }
        }
    }

    int selectedSource = processor->getSelectedStimSource();

    if (selectedSource != -1)
    {
        // LOGC ("Selected source", selectedSource);

        TrackingSources& source = processor->getTrackingSource (selectedSource);
        Colour source_colour = color_palette[source.color];
        g.setColour (source_colour);

        // LOGC ("m_positions size : ", m_positions[selectedSource].size());
        // Plot trajectory as lines
        if (m_positions[selectedSource].size() >= 2)
        {
            for (auto it = m_positions[selectedSource].begin() + 1; it != m_positions[selectedSource].end(); it++)
            {
                TrackingPosition position = *it;
                TrackingPosition prev_position = *(it - 1);

                // std::cout << "Point" << std::endl;

                // if tracking data are empty positions are set to -1
                if (prev_position.x != -1 && prev_position.y != -1)
                {
                    float x = position.x;
                    float y = position.y;
                    float x_prev = prev_position.x;
                    float y_prev = prev_position.y;
                    g.drawLine (x_prev, y_prev, x, y, 5.0f);
                    // std::cout << "Drawing line " << x_prev << ", " << y_prev << ", " << x << ", " << y << std::endl;
                }
                else
                {
                    // std::cout << "Not drawing line" << std::endl;
                }
            }
            // Plot current position as ellipse
            if (! m_positions[selectedSource].empty())
            {
                TrackingPosition position = m_positions[selectedSource].back();
                float x = position.x;
                float y = position.y;

                g.fillEllipse (x - 0.01, y - 0.01, 0.02, 0.02);

                // std::cout << "Drawing point." << std::endl;
            }
            else
            {
                // std::cout << "Not drawing point" << std::endl;
            }
        }
    }

    // Draw moving, creating, copying or resizing circle
    if (m_creatingNewCircle || m_movingCircle || m_resizing || m_copy)
    {
        // draw circle increasing in size
        int x_c, y_c, x, y, radx, rady;

        x_c = int (m_newX);
        y_c = int (m_newY);
        radx = int (m_tempRad);
        rady = int (m_tempRad);
        // center ellipse
        x = x_c - radx;
        y = y_c - rady;

        if (processor->getStimMode() == uniform || processor->getStimMode() == ttl)
            g.setColour (circleColour);
        else
        {
            ColourGradient Cgrad = ColourGradient (Colours::darkorange, double (x_c), double (y_c), Colours::lightyellow, double (x_c + radx), double (y_c + rady), true);
            g.setGradientFill (Cgrad);
        }

        g.fillEllipse (x, y, 2 * radx, 2 * rady);
    }
}

void DisplayAxes::clear()
{
    for (int i = 0; i < MAX_SOURCES; i++)
        m_positions[i].clear();
}

void DisplayAxes::mouseMove (const MouseEvent& event)
{
    if (m_copy)
    {
        m_newX = float (event.x) / float (getWidth());
        m_newY = float (event.y) / float (getHeight());

        // Check boundaries
        if (! (m_newX <= 1 && m_newX >= 0) || ! (m_newY <= 1 && m_newY >= 0))
            m_copy = false;
        repaint();
    }
    else
    {
        int circleIn = processor->isPositionWithinCircles (float (event.x) / float (getWidth()),
                                                           float (event.y) / float (getHeight()));
        if (circleIn != -1)
        {
            if (event.mods.isCtrlDown())
                setMouseCursor (MouseCursor::BottomRightCornerResizeCursor);
            else
                setMouseCursor (MouseCursor::PointingHandCursor);
        }
        else
            setMouseCursor (MouseCursor::CrosshairCursor);
    }
}

void DisplayAxes::mouseEnter (const MouseEvent& event)
{
    setMouseCursor (MouseCursor::CrosshairCursor);
}

void DisplayAxes::mouseExit (const MouseEvent& event)
{
    m_movingCircle = false;
    m_creatingNewCircle = false;
    m_mayBeMoving = false;
    m_resizing = false;
}

void DisplayAxes::mouseDown (const MouseEvent& event)
{
    if (! event.mods.isRightButtonDown())
    {
        // check previous click time
        int64 current = Time::currentTimeMillis();
        int circleIn = processor->isPositionWithinCircles (float (event.x) / float (getWidth()),
                                                           float (event.y) / float (getHeight()));
        // If on a circle -> select circle!
        if (circleIn != -1)
        {
            canvas->selectCircle (circleIn);
            m_creatingNewCircle = false;
            m_newX = float (event.x) / float (getWidth());
            m_newY = float (event.y) / float (getHeight());
            // m_mayBeMoving = true;

            if (event.mods.isCtrlDown())
            {
                m_resizing = true;
            }
            else
            {
                m_mayBeMoving = true;
                m_resizing = false;
                click_time = Time::currentTimeMillis();
            }
        }
        else if (m_copy)
        {
            paste();
        }
        else // Else -> create new circle (set center and drag radius)
        {
            m_creatingNewCircle = true;
            m_newX = float (event.x) / float (getWidth());
            m_newY = float (event.y) / float (getHeight());
            m_tempRad = 0.005;
            setMouseCursor (MouseCursor::UpDownLeftRightResizeCursor);
        }
    }
}

void DisplayAxes::mouseUp (const MouseEvent& event)
{
    if (m_movingCircle)
    {
        // Change to new center and edit circle
        m_newX = float (event.x) / float (getWidth());
        m_newY = float (event.y) / float (getHeight());
        m_newRad = processor->getCircles()[processor->getSelectedCircle()].getRad();

        canvas->editSelectedCircle (m_newX, m_newY, m_newRad);
        m_movingCircle = false;
        // m_doubleClick = false;
        setMouseCursor (MouseCursor::CrosshairCursor);
    }
    else if (m_creatingNewCircle)
    {
        m_creatingNewCircle = false;

        if (event.mouseWasDraggedSinceMouseDown())
        {
            m_newRad = sqrt ((pow (float (event.x) / float (getWidth()) - m_newX, 2) + pow (float (event.y) / float (getHeight()) - m_newY, 2)));

            if (m_newRad > 0.005)
            {
                // Add new circle
                canvas->createCircle (m_newX, m_newY, m_newRad);
            }
        }
        setMouseCursor (MouseCursor::CrosshairCursor);
    }
    else if (m_resizing)
    {
        m_newRad = sqrt ((pow (float (event.x) / float (getWidth()) - m_newX, 2) + pow (float (event.y) / float (getHeight()) - m_newY, 2)));
        m_newX = processor->getCircles()[processor->getSelectedCircle()].getX();
        m_newY = processor->getCircles()[processor->getSelectedCircle()].getY();

        if (m_newRad > 0.005)
        {
            // Add new circle
            // canvas->setOnButton();
            canvas->editSelectedCircle (m_newX, m_newY, m_newRad);
        }
        setMouseCursor (MouseCursor::CrosshairCursor);
    }
}

void DisplayAxes::mouseDrag (const MouseEvent& event)
{
    if (m_mayBeMoving)
    {
        // if dragging is grater than 0.05 -> start moving circle
        // compute m_tempRad
        float cx = 0, cy = 0;
        if (canvas->areThereCicles())
        {
            cx = processor->getCircles()[processor->getSelectedCircle()].getX();
            cy = processor->getCircles()[processor->getSelectedCircle()].getY();
        }
        m_tempRad = sqrt ((pow (float (event.x) / float (getWidth()) - cx, 2) + pow (float (event.y) / float (getHeight()) - cy, 2)));

        if (m_tempRad > 0.02)
        {
            m_movingCircle = true;
            m_mayBeMoving = false;
            setMouseCursor (MouseCursor::DraggingHandCursor);
        }
    }

    if (m_movingCircle)
    {
        m_newX = float (event.x) / float (getWidth());
        m_newY = float (event.y) / float (getHeight());

        if (canvas->areThereCicles())
            m_tempRad = processor->getCircles()[processor->getSelectedCircle()].getRad();

        // Check boundaries
        if (! (m_newX <= 1 && m_newX >= 0) || ! (m_newY <= 1 && m_newY >= 0))
            m_movingCircle = false;
        repaint();
    }
    else if (m_creatingNewCircle)
    {
        // compute m_tempRad
        m_tempRad = sqrt ((pow (float (event.x) / float (getWidth()) - m_newX, 2) + pow (float (event.y) / float (getHeight()) - m_newY, 2)));
        repaint();
    }
    else if (m_resizing)
    {
        // compute m_tempRad
        m_tempRad = sqrt ((pow (float (event.x) / float (getWidth()) - m_newX, 2) + pow (float (event.y) / float (getHeight()) - m_newY, 2)));
        if (canvas->areThereCicles())
        {
            m_newX = processor->getCircles()[processor->getSelectedCircle()].getX();
            m_newY = processor->getCircles()[processor->getSelectedCircle()].getY();
        }
        repaint();
    }
}

void DisplayAxes::copy()
{
    m_copy = true;
    if (canvas->areThereCicles())
        m_newRad = processor->getCircles()[processor->getSelectedCircle()].getRad();
}

void DisplayAxes::paste()
{
    m_copy = false;
    canvas->createCircle (m_newX, m_newY, m_newRad);
}
