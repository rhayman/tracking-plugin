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

#ifndef TRACKINGSTIMULATORCANVAS_H
#define TRACKINGSTIMULATORCANVAS_H

#include <VisualizerWindowHeaders.h>
#include "TrackingNodeEditor.h"
#include "TrackingNode.h"

class DisplayAxes;

/**

  Visualizer class for TrackingNode

*/
class TrackingStimulatorCanvas : public Visualizer,
        public Button::Listener,
        public Label::Listener,
        public ComboBox::Listener,
        public KeyListener
{
public:
    TrackingStimulatorCanvas(TrackingNode* TrackingNode);
    ~TrackingStimulatorCanvas();

    friend class DisplayAxes;

    void paint (Graphics&);
    void resized();
    void clear();
    void initButtons();
    void initLabels();

    // KeyListener interface
    bool keyPressed(const KeyPress &key, Component *originatingComponent) override;

    // Listener interface
    void buttonClicked(Button* button) override;
    void labelTextChanged(Label *label) override;
    void comboBoxChanged(ComboBox* comboBox) override;

    // Visualizer interface
    void refreshState() override;
    void update() override;
    void refresh() override;
    void beginAnimation() override;
    void endAnimation() override;


    bool getUpdateCircle();
    void setUpdateCircle(bool onoff);
    bool areThereCicles();
    void setOnButton();
    float my_round(float x);
    void uploadCircles();
    int getSelectedSource() const;

private:
    TrackingNode* processor;

    float m_width;
    float m_height;

    float m_current_cx;
    float m_current_cy;
    float m_current_crad;

    bool m_onoff;
    bool m_updateCircle;
    bool m_isDeleting;

    int selectedSource;
    int outputChan;

    Colour buttonTextColour;
    Colour labelColour;
    Colour labelTextColour;
    Colour labelBackgroundColour;

    ScopedPointer<DisplayAxes> displayAxes;
    // *** Maybe adjust with proper accessors instead of keep public *** //
    ScopedPointer<UtilityButton> clearButton;
    ScopedPointer<UtilityButton> saveButton;
    ScopedPointer<UtilityButton> saveAsButton;
    ScopedPointer<UtilityButton> loadButton;
    ScopedPointer<UtilityButton> newButton;
    ScopedPointer<UtilityButton> editButton;
    ScopedPointer<UtilityButton> delButton;
    ScopedPointer<UtilityButton> onButton;
    ScopedPointer<UtilityButton> newcircButton;
    ScopedPointer<UtilityButton> circlesButton[MAX_CIRCLES];
    ScopedPointer<UtilityButton> uniformButton;
    ScopedPointer<UtilityButton> gaussianButton;
    ScopedPointer<UtilityButton> ttlButton;

    ScopedPointer<ComboBox> availableSources;
    ScopedPointer<ComboBox> outputChans;

    ScopedPointer<UtilityButton> simTrajectoryButton;

    // Label with non-editable text
    ScopedPointer<Label> sourcesLabel;
    ScopedPointer<Label> outputLabel;
    ScopedPointer<Label> circlesLabel;
    ScopedPointer<Label> paramLabel;
    ScopedPointer<Label> controlLabel;
    ScopedPointer<Label> cxLabel;
    ScopedPointer<Label> cyLabel;
    ScopedPointer<Label> cradLabel;
    ScopedPointer<Label> onLabel;
    ScopedPointer<Label> fmaxLabel;
    ScopedPointer<Label> sdevLabel;
    ScopedPointer<Label> durationLabel;

    // Labels with editable test
    ScopedPointer<Label> cxEditLabel;
    ScopedPointer<Label> cyEditLabel;
    ScopedPointer<Label> cradEditLabel;
    ScopedPointer<Label> fmaxEditLabel;
    ScopedPointer<Label> sdevEditLabel;
    ScopedPointer<Label> durationEditLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingStimulatorCanvas);
};

/**

  Class for displaying and draw circles and current position

*/
class DisplayAxes : public Component
{
public:
    DisplayAxes(TrackingNode* TrackingNode, TrackingStimulatorCanvas* TrackingStimulatorCanvas);
    ~DisplayAxes();

    void addPosition(int index, TrackingPosition& position);

    void paint(Graphics& g);

    void clear();

    void mouseMove(const MouseEvent& event);
    void mouseEnter(const MouseEvent& event);
    void mouseExit(const MouseEvent& event);
    void mouseDown(const MouseEvent& event);
    void mouseUp(const MouseEvent& event);
    void mouseDrag(const MouseEvent& event);

    void copy();
    void paste();


private:

    std::vector<TrackingPosition> m_positions[MAX_SOURCES];

    std::map<String, Colour> color_palette;

    TrackingNode* processor;
    TrackingStimulatorCanvas* canvas;

    Colour circleColour;

    int64 click_time;

    bool m_firstPaint;

    bool m_creatingNewCircle;
    bool m_mayBeMoving;
    bool m_movingCircle;
    bool m_resizing;
    bool m_copy;

    float m_newX;
    float m_newY;
    float m_newRad;
    float m_tempRad;

    MouseCursor::StandardCursorType cursorType;

};

#endif // TRACKINGSTIMULATORCANVAS_H
