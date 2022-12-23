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


TrackingStimulatorCanvas::TrackingStimulatorCanvas(TrackingNode* node)
    : processor(node)
    , m_width(1.0)
    , m_height(1.0)
    , m_updateCircle(true)
    , m_onoff(false)
    , m_isDeleting(true)
    , settingsWidth(250)
    , settingsHeight(500)
    , selectedSource(-1)
    , outputChan(0)
    , labelFont("Fira Sans", "SemiBold", 20.0f)
    , labelTextColour(Colour(255, 200, 0))
    , labelBackgroundColour(Colour(100,100,100))
{
    // Setup buttons
    initButtons();
    // Setup Labels
    initLabels();

    addKeyListener(this);
    setWantsKeyboardFocus(true);
    
    displayAxes = new DisplayAxes(node, this);

    update();
}

TrackingStimulatorCanvas::~TrackingStimulatorCanvas()
{
    removeKeyListener(this);
}

void TrackingStimulatorCanvas::initButtons()
{

    clearButton = new UtilityButton("Clear plot", Font("Small Text", 13, Font::plain));
    clearButton->setRadius(3.0f);
    clearButton->onClick = [this] { clear(); };
    // clearButton->addListener(this);
    addAndMakeVisible(clearButton);

    simTrajectoryButton = new UtilityButton("Simulate", Font("Small Text", 13, Font::plain));
    simTrajectoryButton->setRadius(3.0f);
    simTrajectoryButton->addListener(this);
    simTrajectoryButton->setClickingTogglesState(true);
    addAndMakeVisible(simTrajectoryButton);

    newButton = new UtilityButton("New", Font("Small Text", 13, Font::plain));
    newButton->setRadius(3.0f);
    newButton->addListener(this);
    addAndMakeVisible(newButton);

    editButton = new UtilityButton("Edit", Font("Small Text", 13, Font::plain));
    editButton->setRadius(3.0f);
    editButton->addListener(this);
    addAndMakeVisible(editButton);

    delButton = new UtilityButton("Del", Font("Small Text", 13, Font::plain));
    delButton->setRadius(3.0f);
    delButton->addListener(this);
    addAndMakeVisible(delButton);

    onButton = new TextButton("Disabled");
    onButton->setClickingTogglesState(true);
    onButton->setToggleState(false, dontSendNotification);
    onButton->setColour(TextButton::buttonOnColourId, Colours::green);
    onButton->setColour(TextButton::buttonColourId, Colours::darkred);
    onButton->setColour(TextButton::textColourOnId, Colours::white);
    onButton->setColour(TextButton::textColourOffId, Colours::grey);
    onButton->addListener(this);
    addAndMakeVisible(onButton);

    availableSources = new ComboBox("Tracking Sources");
    availableSources->setEditableText(false);
    availableSources->setJustificationType(Justification::centredLeft);
    availableSources->addListener(this);
    availableSources->setSelectedId(0);
    addAndMakeVisible(availableSources);

    outputChans = new ComboBox("Output Channels");
    outputChans->setEditableText(false);
    outputChans->setJustificationType(Justification::centredLeft);
    outputChans->addListener(this);
    outputChans->setSelectedId(0);

    for (int i=1; i<9; i++)
        outputChans->addItem(String(i), i);

    outputChans->setSelectedId(processor->getOutputChan() + 1, dontSendNotification);
    addAndMakeVisible(outputChans);


    // Create invisible circle toggle button
    for (int i = 0; i<MAX_CIRCLES; i++)
    {
        circlesButton[i] = new UtilityButton(String(i+1), Font("Small Text", 13, Font::plain));
        circlesButton[i]->setRadioGroupId(101);
        circlesButton[i]->setRadius(5.0f);
        circlesButton[i]->addListener(this);
        circlesButton[i]->setClickingTogglesState(true);
        addAndMakeVisible(circlesButton[i]);
    }

    uniformButton = new UtilityButton("uni", Font("Small Text", 13, Font::plain));
    uniformButton->setRadioGroupId(201);
    uniformButton->setRadius(3.0f);
    uniformButton->addListener(this);
    uniformButton->setClickingTogglesState(true);
    addAndMakeVisible(uniformButton);

    gaussianButton = new UtilityButton("gauss", Font("Small Text", 13, Font::plain));
    gaussianButton->setRadioGroupId(201);
    gaussianButton->setRadius(3.0f);
    gaussianButton->addListener(this);
    gaussianButton->setClickingTogglesState(true);
    addAndMakeVisible(gaussianButton);

    ttlButton = new UtilityButton("ttl", Font("Small Text", 13, Font::plain));
    ttlButton->setRadioGroupId(201);
    ttlButton->setRadius(3.0f);
    ttlButton->addListener(this);
    ttlButton->setClickingTogglesState(true);
    addAndMakeVisible(ttlButton);

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
    Font paramFont = Font("Fira Sans", "Regular", 16.0f);
    // Static Labels
    sourcesLabel = new Label("s_sources", "Input Source");
    sourcesLabel->setFont(labelFont);
    addAndMakeVisible(sourcesLabel);

    outputLabel = new Label("s_output", "Output Channel");
    outputLabel->setFont(labelFont);
    addAndMakeVisible(outputLabel);

    circlesLabel = new Label("s_circles", "Circles");
    circlesLabel->setFont(labelFont);
    addAndMakeVisible(circlesLabel);

    paramLabel = new Label("s_param", "Trigger parameters");
    paramLabel->setFont(labelFont);
    addAndMakeVisible(paramLabel);

    fmaxLabel = new Label("s_fmax", "FMax [Hz]:");
    fmaxLabel->setFont(paramFont);
    addAndMakeVisible(fmaxLabel);

    sdevLabel = new Label("s_sdev", "SD [%]:");
    sdevLabel->setFont(paramFont);
    addAndMakeVisible(sdevLabel);

    durationLabel = new Label("s_dur", "Duration [ms]:");
    durationLabel->setFont(paramFont);
    addAndMakeVisible(durationLabel);

    fmaxEditLabel = new Label("fmax", String(processor->getStimFreq()));
    fmaxEditLabel->setFont(paramFont);
    fmaxEditLabel->setColour(Label::textColourId, labelTextColour);
    fmaxEditLabel->setColour(Label::backgroundColourId, labelBackgroundColour);
    fmaxEditLabel->setEditable(true);
    fmaxEditLabel->addListener(this);
    addAndMakeVisible(fmaxEditLabel);

    sdevEditLabel = new Label("sdev", String(processor->getStimSD()));
    sdevEditLabel->setFont(paramFont);
    sdevEditLabel->setColour(Label::textColourId, labelTextColour);
    sdevEditLabel->setColour(Label::backgroundColourId, labelBackgroundColour);
    sdevEditLabel->setEditable(true);
    sdevEditLabel->addListener(this);
    addAndMakeVisible(sdevEditLabel);

    durationEditLabel = new Label("sdev", String(processor->getTTLDuration()));
    durationEditLabel->setFont(paramFont);
    durationEditLabel->setColour(Label::textColourId, labelTextColour);
    durationEditLabel->setColour(Label::backgroundColourId, labelBackgroundColour);
    durationEditLabel->setEditable(true);
    durationEditLabel->addListener(this);
    addAndMakeVisible(durationEditLabel);

    if (processor->getStimMode() == gauss)
    {
        sdevLabel->setVisible(true);
        sdevEditLabel->setVisible(true);
    }
    else
    {
        sdevLabel->setVisible(false);
        sdevEditLabel->setVisible(false);
        fmaxLabel->setVisible(false);
        fmaxEditLabel->setVisible(false);
    }
}

float TrackingStimulatorCanvas::my_round(float x)
{
    return x < 0.0 ? ceil(x - 0.5) : floor(x + 0.5);
}

bool TrackingStimulatorCanvas::areThereCicles()
{
    if (processor->getCircles().size()>0)
        return true;
    else
        return false;
}

void TrackingStimulatorCanvas::paint (Graphics& g)
{
    float plot_height = 0.98*getHeight();
    float plot_width = 0.75*getWidth();
    float plot_bottom_left_x = 0.01*getWidth();
    float plot_bottom_left_y = 0.01*getHeight();

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

    g.setColour(Colours::black); // background color
    g.fillRect(0, 0, getWidth(), getHeight());
        
    g.setColour(Colour(125, 125, 125)); //settings menu background color
    g.fillRoundedRectangle(getWidth() - settingsWidth - 10, 10, settingsWidth, settingsHeight - 20, 7.0f);
        
    g.fillEllipse(getWidth() - 0.065*getWidth(), 0.41*getHeight(), 0.03*getWidth(), 0.03*getHeight());

    displayAxes->setBounds(int(plot_bottom_left_x), int(plot_bottom_left_y),
                           int(camWidth), int(camHeight));
    displayAxes->repaint();
}

void TrackingStimulatorCanvas::resized()
{

    if(0.22*getWidth() < 150)
        settingsWidth = 150;
    else if(0.22*getWidth() > 300)
        settingsWidth = 300;
    else
        settingsWidth = 0.22*getWidth();
    
    if(getHeight() < 500)
        settingsHeight = 500;
    else if(getHeight() > 800)
        settingsHeight = 800;
    else
        settingsHeight = getHeight();

//    displayAxes->setBounds(int(0.01*getHeight()), int(0.01*getHeight()), int(0.98*getHeight()), int(0.98*getHeight()));
    addAndMakeVisible(displayAxes);

    sourcesLabel->setBounds(getWidth() - settingsWidth, 25, settingsWidth - 20, 25);
    availableSources->setBounds(getWidth() - settingsWidth, 60, settingsWidth - 20, 25);

    outputLabel->setBounds(getWidth() - settingsWidth, 100, settingsWidth, 25);
    outputChans->setBounds(getWidth() - settingsWidth, 135, settingsWidth - 20, 25);

    circlesLabel->setBounds(getWidth() - settingsWidth, 0.34*settingsHeight, settingsWidth - 20, 25);
    newButton->setBounds(getWidth() - settingsWidth, 0.39*settingsHeight, settingsWidth / 3 - 5, 30);
    editButton->setBounds(getWidth() - (2*settingsWidth/3) - 5, 0.39*settingsHeight, settingsWidth / 3 - 5, 30);
    delButton->setBounds(getWidth() - (settingsWidth / 3) - 10, 0.39*settingsHeight, settingsWidth / 3 - 5, 30);

    onButton->setBounds(getWidth() - (3*settingsWidth/4), 0.52*settingsHeight, settingsWidth/2 - 30, 30);

    for (int i = 0; i<MAX_CIRCLES; i++)
    {
        circlesButton[i]->setBounds(getWidth() - settingsWidth + i*(settingsWidth - 20)/MAX_CIRCLES, 0.46*settingsHeight,
                                    (settingsWidth - 20)/MAX_CIRCLES, 25);

        if (i<processor->getCircles().size())
            circlesButton[i]->setVisible(true);
        else
            circlesButton[i]->setVisible(false);
    }

    paramLabel->setBounds(getWidth() - settingsWidth, 0.6*settingsHeight, settingsWidth - 20, 25);
    uniformButton->setBounds(getWidth() - settingsWidth, 0.65*settingsHeight, settingsWidth / 3 - 5, 30);
    gaussianButton->setBounds(getWidth() - (2*settingsWidth/3) - 5, 0.65*settingsHeight, settingsWidth / 3 - 5, 30);
    ttlButton->setBounds(getWidth() - (settingsWidth / 3) - 10, 0.65*settingsHeight, settingsWidth / 3 - 5, 30);

    durationLabel->setBounds(getWidth() - settingsWidth, 0.7*settingsHeight, settingsWidth/2 - 20, 30);
    durationEditLabel->setBounds(getWidth() - (settingsWidth / 2), 0.7*settingsHeight,settingsWidth/2 - 20, 30);
    
    fmaxLabel->setBounds(getWidth() - settingsWidth, 0.75*settingsHeight, settingsWidth/2 - 20, 30);
    fmaxEditLabel->setBounds(getWidth() - (settingsWidth / 2), 0.75*settingsHeight, settingsWidth/2 - 20, 30);
    
    sdevLabel->setBounds(getWidth() - settingsWidth, 0.8*settingsHeight, settingsWidth/2 - 20, 30);
    sdevEditLabel->setBounds(getWidth() - (settingsWidth / 2), 0.8*settingsHeight, settingsWidth/2 - 20, 30);

    simTrajectoryButton->setBounds(getWidth() - settingsWidth, settingsHeight - 50, settingsWidth/2 - 20, 30);
    clearButton->setBounds(getWidth() - (settingsWidth / 2), settingsHeight - 50, settingsWidth/2 - 20, 30);

    refresh();
}

void TrackingStimulatorCanvas::comboBoxChanged(ComboBox* comboBox)
{
    if (comboBox == availableSources)
    {
        if (comboBox->getSelectedId() > 1)
        {
            int index = comboBox->getSelectedId() - 2;
            selectedSource = index;
        }
        else
            selectedSource = -1;
        processor->setSelectedStimSource(selectedSource);
    }
    else if (comboBox == outputChans)
    {
        if (comboBox->getSelectedId() > 0)
        {
            int index = comboBox->getSelectedId() - 1;
            outputChan = index;
        }
        else
            outputChan = -1;
        processor->setOutputChan(outputChan);
    }
}



bool TrackingStimulatorCanvas::keyPressed(const KeyPress &key, Component *originatingComponent)
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
    else if (key == KeyPress('c', ModifierKeys::ctrlModifier, 0))
    {
        if (areThereCicles())
        {
            displayAxes->copy();
            return true;
        }
    }
    else if (key == KeyPress('v', ModifierKeys::ctrlModifier, 0))
    {
        if (areThereCicles())
        {
            displayAxes->paste();
            return true;
        }
    }
    return false;
}

void TrackingStimulatorCanvas::buttonClicked(Button* button)
{
    if (button == clearButton)
    {
        clear();
    }
    else if (button == simTrajectoryButton)
    {
        if (simTrajectoryButton->getToggleState() == true)
        {
            processor->setSimulateTrajectory(true);
            m_width = 1;
            m_height = 1;
        }
        else
            processor->setSimulateTrajectory(false);
    }
    else if (button == newButton)
    {
        
        float cx, cy, crad;

        if(processor->getSelectedCircle() >= 0)
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

        CircleEditor* newCircleEditor = new CircleEditor(this, false, cx, cy, crad);
        
        CallOutBox& myBox = CallOutBox::launchAsynchronously (
                                std::unique_ptr<Component>(newCircleEditor), 
                                button->getScreenBounds(), 
                                nullptr);

        myBox.setDismissalMouseClicksAreAlwaysConsumed(true);

    }
    else if (button == editButton)
    {
        if(processor->getSelectedCircle() >= 0)
        {
            float cx = processor->getCircles()[processor->getSelectedCircle()].getX() * 100;
            float cy = processor->getCircles()[processor->getSelectedCircle()].getY() * 100;
            float crad = processor->getCircles()[processor->getSelectedCircle()].getRad() * 100;

            CircleEditor* newCircleEditor = new CircleEditor(this, true, cx, cy, crad);
            
            CallOutBox& myBox = CallOutBox::launchAsynchronously (
                                    std::unique_ptr<Component>(newCircleEditor), 
                                    button->getScreenBounds(), 
                                    nullptr);

            myBox.setDismissalMouseClicksAreAlwaysConsumed(true);
        }
        else
        {
            Label* warningLabel = new Label("warning", "Please select a circle first!");
            warningLabel->setSize(130, 60);
            warningLabel->setColour(Label::textColourId, Colours::white);
            warningLabel->setJustificationType(Justification::centred);

            CallOutBox& myBox = CallOutBox::launchAsynchronously (
                                    std::unique_ptr<Component>(warningLabel), 
                                    button->getScreenBounds(), 
                                    nullptr);
            
            myBox.setDismissalMouseClicksAreAlwaysConsumed(true);
            
        }
    }
    else if (button == delButton)
    {
        m_updateCircle = true;

        processor->deleteCircle(processor->getSelectedCircle());

        // Blank labels and untoggle all circle buttons
        processor->setSelectedCircle(-1);
        m_onoff = false;

        for (int i = 0; i<MAX_CIRCLES; i++)
            //            circlesButton[i]->setEnabledState(false);
            circlesButton[i]->setToggleState(false, true);
        
        // make visible only the remaining labels
        for (int i = 0; i<MAX_CIRCLES; i++)
        {
            if (i<processor->getCircles().size())
                circlesButton[i]->setVisible(true);
            else
                circlesButton[i]->setVisible(false);
        }

    }
    else if (button == onButton)
    {
        // make changes immediately if circle already exist
        if (processor->getSelectedCircle() != -1)
        {
            m_onoff = onButton->getToggleState();
            if(m_onoff)
            {
                onButton->setButtonText("Enabled");
            }
            else
            {
                onButton->setButtonText("Disabled");
            }
            editSelectedCircle(m_current_cx, m_current_cy, m_current_crad);
        }
        else
        {
            onButton->setToggleState(false, dontSendNotification);
            onButton->setButtonText("Disabled");
            m_onoff = false;
        }
    }
    else if (button == uniformButton)
    {
        if (button->getToggleState()==true)
        {
            fmaxLabel->setVisible(true);
            fmaxEditLabel->setVisible(true);
            sdevLabel->setVisible(false);
            sdevEditLabel->setVisible(false);
            processor->setStimMode(uniform);
        }
    }
    else if (button == gaussianButton)
    {
        if (button->getToggleState()==true)
        {
            fmaxLabel->setVisible(true);
            fmaxEditLabel->setVisible(true);
            sdevLabel->setVisible(true);
            sdevEditLabel->setVisible(true);
            processor->setStimMode(gauss);
        }
    }
    else if (button == ttlButton)
    {
        if (button->getToggleState()==true)
        {
            fmaxLabel->setVisible(false);
            fmaxEditLabel->setVisible(false);
            sdevLabel->setVisible(false);
            sdevEditLabel->setVisible(false);
            processor->setStimMode(ttl);
        }
    }
    else if(button->getRadioGroupId() == 101)
    {
        // check if one of circle button has been clicked
        bool someToggled = false;
        for (int i = 0; i<MAX_CIRCLES; i++)
        {
            if (button == circlesButton[i] && circlesButton[i]->getEnabledState() )
            {
                // toggle button and untoggle all the others + update
                if (button->getToggleState()==true)
                {
                    processor->setSelectedCircle(i);
                    someToggled = true;
                    // retrieve labels and on button values
                    if (areThereCicles())
                    {
                        m_onoff = processor->getCircles()[processor->getSelectedCircle()].getOn();
                        m_current_cx = processor->getCircles()[processor->getSelectedCircle()].getX();
                        m_current_cy = processor->getCircles()[processor->getSelectedCircle()].getY();
                        m_current_crad = processor->getCircles()[processor->getSelectedCircle()].getRad();
                        onButton->setToggleState(m_onoff, sendNotification);
                    }
                }
                break;
            }
        }
        if (!someToggled)
        {
            // blank labels
            processor->setSelectedCircle(-1);
            m_onoff = false;

        }
    }
    repaint();
}

void TrackingStimulatorCanvas::uploadCircles()
{
    // circle buttons visible
    for (int i = 0; i<MAX_CIRCLES; i++)
    {
        if (i<processor->getCircles().size())
            circlesButton[i]->setVisible(true);
        else
            circlesButton[i]->setVisible(false);
    }
}

void TrackingStimulatorCanvas::labelTextChanged(Label *label)
{
    if (label == fmaxEditLabel)
    {
        Value val = label->getTextValue();
        if ((float(val.getValue())>=0 && float(val.getValue())<=10000))
            processor->setStimFreq(float(val.getValue()));
        else
        {
            CoreServices::sendStatusMessage("Selected values cannot be negative!");
            label->setText("", dontSendNotification);
        }
    }
    if (label == sdevEditLabel)
    {
        Value val = label->getTextValue();
        if ((float(val.getValue())>=0 && float(val.getValue())<=1))
            if (float(val.getValue())>0)
                processor->setStimSD(float(val.getValue()));
            else
                processor->setStimSD(1e-10);
        else
        {
            CoreServices::sendStatusMessage("Selected values must be between 0 and 1!");
            label->setText("", dontSendNotification);
        }
    }
    if (label == durationEditLabel)
    {
        Value val = label->getTextValue();
        if (int(val.getValue())>=0)
            processor->setTTLDuration(int(val.getValue()));
        else
        {
            CoreServices::sendStatusMessage("Selected values cannot be negative!");
            label->setText("", dontSendNotification);
        }
    }
}


void TrackingStimulatorCanvas::refreshState()
{
}

void TrackingStimulatorCanvas::update()
{
    availableSources->clear();
    int nSources = processor->getNumSources();
    int nextItem = 2;
    availableSources->addItem("SELECT", 1);
    for (int i = 0; i < nSources; i++)
    {
        TrackingSources& source = processor->getTrackingSource(i);
        String name = source.name;
        availableSources->addItem(name, nextItem++);
    }
    availableSources->setSelectedId(processor->getSelectedStimSource()+2); //first is SELECT
}

void TrackingStimulatorCanvas::refresh()
{

    if (processor->positionIsUpdated())
    {
        for (int i = 0; i < processor->getNumSources(); i++)
        {
            TrackingSources& source = processor->getTrackingSource(i);
            TrackingPosition currPos;
            currPos.x = source.x_pos;
            currPos.y = source.y_pos;
            currPos.width = source.width;
            currPos.height = source.height;

            displayAxes->addPosition(i, currPos);

            // for now, just pick one w and h
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

void TrackingStimulatorCanvas::createCircle(float cx, float cy, float rad)
{
    if (processor->getCircles().size() < MAX_CIRCLES)
    {
        setOnButton();
        processor->addCircle(StimCircle(cx, cy, rad, m_onoff));
        processor->setSelectedCircle(processor->getCircles().size()-1);
        circlesButton[processor->getSelectedCircle()]->setVisible(true);

        // toggle current circle button (untoggles all the others)
        if (circlesButton[processor->getSelectedCircle()]->getToggleState()==false)
            circlesButton[processor->getSelectedCircle()]->triggerClick();

        m_isDeleting = false;
        m_updateCircle = true;
    }
    else
        CoreServices::sendStatusMessage("Max number of circles reached!");
}

void TrackingStimulatorCanvas::selectCircle(int circleIdx)
{
    if (!circlesButton[circleIdx]->getToggleState())
        circlesButton[circleIdx]->triggerClick();
}

void TrackingStimulatorCanvas::editSelectedCircle(float cx, float cy, float rad)
{
    if (areThereCicles())
        processor->editCircle(processor->getSelectedCircle(), cx, cy, rad, m_onoff);
    
    m_updateCircle = true;
}

bool TrackingStimulatorCanvas::getUpdateCircle(){
    return m_updateCircle;
}

void TrackingStimulatorCanvas::setUpdateCircle(bool onoff){
    m_updateCircle = onoff;
}

void TrackingStimulatorCanvas::setOnButton()
{
    m_onoff = true;
}



/** ------------- Circle Editor Component --------------- */

CircleEditor::CircleEditor(TrackingStimulatorCanvas* stimCanvas, bool isEditMode_, float cx, float cy, float cRad)
    : canvas(stimCanvas)
    , isEditMode(isEditMode_)
    , xVal(cx)
    , yVal(cy)
    , radius(cRad)
{
    LookAndFeel_V4* sliderLAF = new LookAndFeel_V4();
    sliderLAF->setColour(Slider::textBoxBackgroundColourId, Colours::lightgrey);
    sliderLAF->setColour(Slider::textBoxTextColourId, Colours::black);
    sliderLAF->setColour(Slider::textBoxHighlightColourId, Colours::darkgrey);
    sliderLAF->setColour(Slider::trackColourId, Colours::grey);
    sliderLAF->setColour(Slider::thumbColourId, Colours::yellow);
    sliderLAF->setColour(Slider::backgroundColourId, Colours::black);

    Font labelFont = Font("Fira Sans", "SemiBold", 16.0f);

    cxLabel = std::make_unique<Label>("CX", "X [%] :");
    cxLabel->setFont(labelFont);
    cxLabel->setColour(Label::textColourId, Colours::lightgrey);
    cxLabel->setBounds(10, 10, 55, 30);
    addAndMakeVisible(cxLabel.get());

    cxSlider = std::make_unique<Slider>("CXSlider");
    cxSlider->setLookAndFeel(sliderLAF);
    cxSlider->setSliderStyle(Slider::LinearHorizontal);
    cxSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 40, 25);
    cxSlider->setRange(0, 100, 1);
    cxSlider->setValue(xVal, dontSendNotification);
    cxSlider->setBounds(70, 10, 160, 30);
    addAndMakeVisible(cxSlider.get());

    cyLabel = std::make_unique<Label>("CY", "Y [%] :");
    cyLabel->setFont(labelFont);
    cyLabel->setColour(Label::textColourId, Colours::lightgrey);
    cyLabel->setBounds(10, 50, 55, 30);
    addAndMakeVisible(cyLabel.get());

    cySlider = std::make_unique<Slider>("CYSlider");
    cySlider->setLookAndFeel(sliderLAF);
    cySlider->setSliderStyle(Slider::LinearHorizontal);
    cySlider->setTextBoxStyle(Slider::TextBoxLeft, false, 40, 25);
    cySlider->setRange(0, 100, 1);
    cySlider->setValue(yVal, dontSendNotification);
    cySlider->setBounds(70, 50, 160, 30);
    addAndMakeVisible(cySlider.get());

    cradLabel = std::make_unique<Label>("CRAD", "Rad [%] :");
    cradLabel->setFont(labelFont);
    cradLabel->setColour(Label::textColourId, Colours::lightgrey);
    cradLabel->setBounds(10, 90, 55, 30);
    addAndMakeVisible(cradLabel.get());

    cradSlider = std::make_unique<Slider>("CRadSlider");
    cradSlider->setLookAndFeel(sliderLAF);
    cradSlider->setSliderStyle(Slider::LinearHorizontal);
    cradSlider->setTextBoxStyle(Slider::TextBoxLeft, false, 40, 25);
    cradSlider->setRange(1, 100, 1);
    cradSlider->setValue(radius, dontSendNotification);
    cradSlider->setBounds(70, 90, 160, 30);
    addAndMakeVisible(cradSlider.get());

    if(isEditMode)
    {   
        cxSlider->onValueChange = [this] { updateCircleParams(); };
        cySlider->onValueChange = [this] { updateCircleParams(); };
        cradSlider->onValueChange = [this] { updateCircleParams(); };
        setSize(240, 130);
    }
    else
    {
        createButton = std::make_unique<TextButton>("Create");
        createButton->setButtonText("Create");
        createButton->setColour(TextButton::buttonColourId, Colours::green);
        createButton->setColour(TextButton::textColourOffId , Colours::white);
        createButton->setBounds(85, 140, 70, 30);
        addAndMakeVisible(createButton.get());
        createButton->onClick = [this] { createNewCircle(); };

        setSize(240, 180);
    }
}

void CircleEditor::updateCircleParams()
{
    xVal = cxSlider->getValue() / 100;
    yVal = cySlider->getValue() / 100;
    radius = cradSlider->getValue() / 100;

    canvas->editSelectedCircle(xVal, yVal, radius);
}

void CircleEditor::createNewCircle()
{
    xVal = cxSlider->getValue() / 100;
    yVal = cySlider->getValue() / 100;
    radius = cradSlider->getValue() / 100;

    canvas->createCircle(xVal, yVal, radius);

    findParentComponentOfClass<CallOutBox>()->exitModalState(0);
}


/** ------------- Canvas Display Axes --------------- */

DisplayAxes::DisplayAxes(TrackingNode* node, TrackingStimulatorCanvas* canvas_)
    : processor(node)
    , canvas(canvas_)
    , circleColour(Colour(225,200,30))
    , m_creatingNewCircle(false)
    , m_movingCircle(false)
    , m_mayBeMoving(false)
    , m_resizing(false)
    , m_firstPaint(true)
    , m_copy (false)

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

DisplayAxes::~DisplayAxes(){}


void DisplayAxes::addPosition(int index, TrackingPosition& postionData)
{
    m_positions[index].push_back(postionData);
}


void DisplayAxes::paint(Graphics& g)
{

    g.setColour(Colour(0, 18, 43)); //background color
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


            x_c = int(cur_x * getWidth());
            y_c = int(cur_y * getHeight());

            radx = int(cur_rad * getWidth());
            rady = int(cur_rad * getHeight());
            // center ellipse
            x = x_c - radx;
            y = y_c - rady;

            // draw circle if it is ON
            if (processor->getCircles()[i].getOn())
            {
                if (processor->getStimMode() == uniform || processor->getStimMode() == ttl)
                            g.setColour(circleColour);
                else
                {
                    ColourGradient Cgrad = ColourGradient(Colours::darkorange, double(x_c), double(y_c),
                                                            Colours::lightyellow, double(x_c+radx), double(y_c+rady), true);
                    g.setGradientFill(Cgrad);
                }
            }
            else
                g.setColour(Colours::grey.withAlpha(0.5f));

            if (i==processor->getSelectedCircle())
            {
                // if circle is being moved or changed size, don't draw static circle
                if (!(m_movingCircle || m_resizing))
                {
                    g.fillEllipse(x, y, 2*radx, 2*rady);
                    g.setColour(Colours::white);
                    g.drawEllipse(x, y, 2*radx, 2*rady, 3.0f);

                    // Display circle number
                    g.setColour(Colours::white);
                    g.setFont(18.0f);
                    g.drawRect(x_c - 10, y_c - 10, 20, 20, 2);
                    g.drawFittedText(String(i+1), x_c - 10, y_c - 10, 20, 20, Justification::centred, 1);
                }
            }
            else
            {
                g.fillEllipse(x, y, 2*radx, 2*rady);

                // Display circle number
                g.setColour(Colours::white);
                g.setFont(18.0f);
                g.drawFittedText(String(i+1), x_c - 10, y_c - 10, 20, 20, Justification::centred, 1);
            }
        }
    }

    int selectedSource = processor->getSelectedStimSource();

    if(selectedSource != -1)
    {
        TrackingSources& source = processor->getTrackingSource(selectedSource);
        Colour source_colour = color_palette[source.color];
        g.setColour(source_colour);

        // Plot trajectory as lines
        if (m_positions[selectedSource].size () >= 2)
        {
            for(auto it = m_positions[selectedSource].begin()+1; it != m_positions[selectedSource].end(); it++)
            {
                TrackingPosition position = *it;
                TrackingPosition prev_position = *(it-1);

                // if tracking data are empty positions are set to -1
                if (prev_position.x != -1 && prev_position.y != -1)
                {
                    float x = getWidth()*position.x;
                    float y = getHeight()*position.y;
                    float x_prev = getWidth()*prev_position.x;
                    float y_prev = getHeight()*prev_position.y;
                    g.drawLine(x_prev, y_prev, x, y, 5.0f);
                }
            }
            // Plot current position as ellipse
            if (!m_positions[selectedSource].empty ())
            {
                TrackingPosition position = m_positions[selectedSource].back();
                float x = getWidth()*position.x;
                float y = getHeight()*position.y;

                g.fillEllipse(x - 0.01*getHeight(), y - 0.01*getHeight(), 0.02*getHeight(), 0.02*getHeight());
            }
        }
    }

    // Draw moving, creating, copying or resizing circle
    if (m_creatingNewCircle || m_movingCircle || m_resizing || m_copy)
    {
        // draw circle increasing in size
        int x_c, y_c, x, y, radx, rady;

        x_c = int(m_newX * getWidth());
        y_c = int(m_newY * getHeight());
        radx = int(m_tempRad * getWidth());
        rady = int(m_tempRad * getHeight());
        // center ellipse
        x = x_c - radx;
        y = y_c - rady;

        if (processor->getStimMode() == uniform || processor->getStimMode() == ttl)
            g.setColour(circleColour);
        else
        {
            ColourGradient Cgrad = ColourGradient(Colours::darkorange, double(x_c), double(y_c),
                                                  Colours::lightyellow, double(x_c+radx), double(y_c+rady), true);
            g.setGradientFill(Cgrad);
        }

        g.fillEllipse(x, y, 2*radx, 2*rady);

    }
}

void DisplayAxes::clear()
{
    for (int i = 0; i<MAX_SOURCES; i++)
        m_positions[i].clear();
}

void DisplayAxes::mouseMove(const MouseEvent& event){
    
    if (m_copy)
    {
        m_newX = float(event.x)/float(getWidth());
        m_newY = float(event.y)/float(getHeight());

        // Check boundaries
        if (!(m_newX <= 1 && m_newX >= 0) || !(m_newY <= 1 && m_newY >= 0))
            m_copy = false;
        repaint();
    }
    else
    {
        int circleIn = processor->isPositionWithinCircles(float(event.x)/float(getWidth()),
                                                          float(event.y)/float(getHeight()));
        if(circleIn != -1)
        {
            if (event.mods.isCtrlDown())
                setMouseCursor(MouseCursor::BottomRightCornerResizeCursor);
            else
                setMouseCursor(MouseCursor::PointingHandCursor);
        }
        else
            setMouseCursor(MouseCursor::CrosshairCursor);
    }

}

void DisplayAxes::mouseEnter(const MouseEvent& event){
    setMouseCursor(MouseCursor::CrosshairCursor);
}

void DisplayAxes::mouseExit(const MouseEvent& event){
    m_movingCircle = false;
    m_creatingNewCircle = false;
    m_mayBeMoving = false;
    m_resizing = false;
}

void DisplayAxes::mouseDown(const MouseEvent& event)
{
    if (!event.mods.isRightButtonDown())
    {
        // check previous click time
        int64 current = Time::currentTimeMillis();
        int circleIn = processor->isPositionWithinCircles(float(event.x)/float(getWidth()),
                                                          float(event.y)/float(getHeight()));
        // If on a circle -> select circle!
        if (circleIn != -1)
        {
            canvas->selectCircle(circleIn);
            m_creatingNewCircle = false;
            m_newX = float(event.x)/float(getWidth());
            m_newY = float(event.y)/float(getHeight());
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
        else if(m_copy)
        {
            paste();
        }
        else  // Else -> create new circle (set center and drag radius)
        {
            m_creatingNewCircle = true;
            m_newX = float(event.x)/float(getWidth());
            m_newY = float(event.y)/float(getHeight());
            m_tempRad = 0.005;
            setMouseCursor(MouseCursor::UpDownLeftRightResizeCursor);
        }
    }
}

void DisplayAxes::mouseUp(const MouseEvent& event){
    if (m_movingCircle)
    {
        // Change to new center and edit circle
        m_newX = float(event.x)/float(getWidth());
        m_newY = float(event.y)/float(getHeight());
        m_newRad = processor->getCircles()[processor->getSelectedCircle()].getRad();

        canvas->editSelectedCircle(m_newX, m_newY, m_newRad);
        m_movingCircle = false;
        // m_doubleClick = false;
        setMouseCursor(MouseCursor::CrosshairCursor);
    }
    else if (m_creatingNewCircle)
    {
        m_creatingNewCircle = false;

        if(event.mouseWasDraggedSinceMouseDown())
        {

            m_newRad = sqrt((pow(float(event.x)/float(getWidth())-m_newX, 2) +
                                pow(float(event.y)/float(getHeight())-m_newY, 2)));

            if (m_newRad > 0.005)
            {
                // Add new circle
                canvas->createCircle(m_newX, m_newY, m_newRad);
            }
        }
        setMouseCursor(MouseCursor::CrosshairCursor);
    }
    else if (m_resizing)
    {
        m_newRad = sqrt((pow(float(event.x)/float(getWidth())-m_newX, 2) +
                             pow(float(event.y)/float(getHeight())-m_newY, 2)));
        m_newX = processor->getCircles()[processor->getSelectedCircle()].getX();
        m_newY = processor->getCircles()[processor->getSelectedCircle()].getY();

        if (m_newRad > 0.005)
        {
            // Add new circle
            // canvas->setOnButton();
            canvas->editSelectedCircle(m_newX, m_newY, m_newRad);
        }
        setMouseCursor(MouseCursor::CrosshairCursor);
    }
}

void DisplayAxes::mouseDrag(const MouseEvent& event){
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
        m_tempRad = sqrt((pow(float(event.x)/float(getWidth())-cx, 2) +
                          pow(float(event.y)/float(getHeight())-cy, 2)));

        if (m_tempRad > 0.02)
        {
            m_movingCircle = true;
            m_mayBeMoving = false;
            setMouseCursor(MouseCursor::DraggingHandCursor);
        }
    }

    if (m_movingCircle)
    {
        m_newX = float(event.x)/float(getWidth());
        m_newY = float(event.y)/float(getHeight());

        if (canvas->areThereCicles())
            m_tempRad = processor->getCircles()[processor->getSelectedCircle()].getRad();

        // Check boundaries
        if (!(m_newX <= 1 && m_newX >= 0) || !(m_newY <= 1 && m_newY >= 0))
            m_movingCircle = false;
        repaint();
    }
    else if (m_creatingNewCircle)
    {
        // compute m_tempRad
        m_tempRad = sqrt((pow(float(event.x)/float(getWidth())-m_newX, 2) +
                          pow(float(event.y)/float(getHeight())-m_newY, 2)));
        repaint();
    }
    else if (m_resizing)
    {
        // compute m_tempRad
        m_tempRad = sqrt((pow(float(event.x)/float(getWidth())-m_newX, 2) +
                          pow(float(event.y)/float(getHeight())-m_newY, 2)));
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
    canvas->createCircle(m_newX, m_newY, m_newRad);

}



