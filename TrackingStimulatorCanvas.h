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

#include "TrackingNode.h"
#include "TrackingNodeEditor.h"
#include <VisualizerWindowHeaders.h>

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
    TrackingStimulatorCanvas (GenericProcessor* sourceNode, TrackingNode* thread);
    ~TrackingStimulatorCanvas();

    void paint (Graphics&);
    void resized();
    void clear();
    void initButtons();
    void initLabels();

    // KeyListener interface
    bool keyPressed (const KeyPress& key, Component* originatingComponent) override;

    // Listener interface
    void buttonClicked (Button* button) override;
    void labelTextChanged (Label* label) override;
    void comboBoxChanged (ComboBox* comboBox) override;

    // Visualizer interface
    void refreshState() override {};
    void updateSettings() override;
    void refresh() override;
    void beginAnimation() override;
    void endAnimation() override;

    void saveCustomParametersToXml (XmlElement* xml) override;
    void loadCustomParametersFromXml (XmlElement* xml) override;

    void createCircle (float xVal, float yVal, float rad);
    void selectCircle (int circle);
    void editSelectedCircle (float xVal, float yVal, float rad);
    bool getUpdateCircle();
    void setUpdateCircle (bool onoff);
    bool areThereCicles();
    void setOnButton();
    float my_round (float x);
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

    int settingsWidth;
    int settingsHeight;

    int selectedSource;
    int outputChan;

    FontOptions sectionLabelFont;

    std::unique_ptr<DisplayAxes> displayAxes;
    std::unique_ptr<UtilityButton> clearButton;
    std::unique_ptr<UtilityButton> newButton;
    std::unique_ptr<UtilityButton> editButton;
    std::unique_ptr<UtilityButton> delButton;
    std::unique_ptr<TextButton> onButton;
    std::unique_ptr<UtilityButton> circlesButton[MAX_CIRCLES];
    std::unique_ptr<UtilityButton> uniformButton;
    std::unique_ptr<UtilityButton> gaussianButton;
    std::unique_ptr<UtilityButton> ttlButton;

    std::unique_ptr<ComboBox> availableSources;
    std::unique_ptr<ComboBox> outputChans;

    std::unique_ptr<UtilityButton> simTrajectoryButton;

    // Label with non-editable text
    std::unique_ptr<Label> sourcesLabel;
    std::unique_ptr<Label> outputLabel;
    std::unique_ptr<Label> circlesLabel;
    std::unique_ptr<Label> paramLabel;
    std::unique_ptr<Label> onLabel;
    std::unique_ptr<Label> fmaxLabel;
    std::unique_ptr<Label> sdevLabel;
    std::unique_ptr<Label> durationLabel;

    std::unique_ptr<CustomTextBox> fmaxEditLabel;
    std::unique_ptr<CustomTextBox> sdevEditLabel;
    std::unique_ptr<CustomTextBox> durationEditLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackingStimulatorCanvas);
};

/**

  User interface for the creating and editing circles.

  Allows the user to create and edit circles by sepcifying 
  the x, y, and radius values.

  @see TrackingStimulatorCanvas

*/
class CircleEditor : public Component
{
public:
    CircleEditor (TrackingStimulatorCanvas* stimCanvas, bool isEditMode, float cx, float cy, float cRad);

    ~CircleEditor();

    void paint (Graphics& g) override;

private:
    std::unique_ptr<Slider> cxSlider;
    std::unique_ptr<Slider> cySlider;
    std::unique_ptr<Slider> cradSlider;

    std::unique_ptr<Label> cxLabel;
    std::unique_ptr<Label> cyLabel;
    std::unique_ptr<Label> cradLabel;

    std::unique_ptr<TextButton> createButton;

    float xVal;
    float yVal;
    float radius;

    bool isEditMode;

    TrackingStimulatorCanvas* canvas;

    void updateCircleParams();

    void createNewCircle();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircleEditor);
};

/**

  Class for displaying and draw circles and current position

*/
class DisplayAxes : public Component
{
public:
    DisplayAxes (TrackingNode* TrackingNode, TrackingStimulatorCanvas* TrackingStimulatorCanvas);
    ~DisplayAxes();

    void addPosition (int index, TrackingPosition& position);

    void paint (Graphics& g);

    void clear();

    void mouseMove (const MouseEvent& event);
    void mouseEnter (const MouseEvent& event);
    void mouseExit (const MouseEvent& event);
    void mouseDown (const MouseEvent& event);
    void mouseUp (const MouseEvent& event);
    void mouseDrag (const MouseEvent& event);

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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DisplayAxes);
};

#endif // TRACKINGSTIMULATORCANVAS_H
