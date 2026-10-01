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

#ifndef TRACKINGNODEEDITOR_H
#define TRACKINGNODEEDITOR_H

#define MAX_SOURCES 10

#include <VisualizerEditorHeaders.h>

class TrackingNode;

class TrackingNodeEditor : public VisualizerEditor,
                           public ComboBox::Listener,
                           public Button::Listener
{
public:
    /** Constructor */
    TrackingNodeEditor (GenericProcessor* parentNode);

    /** Destructor */
    ~TrackingNodeEditor() {}

    Visualizer* createNewCanvas() override;

    // combo box listener
    void comboBoxChanged (ComboBox* comboBox) override;

    // button listener
    void buttonClicked (Button* button) override;

    /** Saves tracking node editor parameters */
    void saveVisualizerEditorParameters (XmlElement* xml) override;

    /** Loads tracking node editor parameters */
    void loadVisualizerEditorParameters (XmlElement* xml) override;

    int getPort() const;
    String getAddress() const;
    String getColor() const;

private:
    /** Pointer to the TrackingNode processor */
    TrackingNode* node;

    std::unique_ptr<UtilityButton> plusButton;
    std::unique_ptr<UtilityButton> minusButton;
    std::unique_ptr<Label> sourceLabel;
    std::unique_ptr<ComboBox> trackingSourceSelector;

    std::unique_ptr<Label> portLabel;
    std::unique_ptr<CustomTextBox> portEditor;
    std::unique_ptr<Label> addressLabel;
    std::unique_ptr<CustomTextBox> addressEditor;
    std::unique_ptr<Label> colorLabel;
    std::unique_ptr<ComboBox> colorSelector;

    int selectedSource = -1;

    int port;
    String address;

    /** Updates the editor's UI components to match the current state */
    void updateCustomView() override;

    /** Generates an assertion if this class leaks */
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackingNodeEditor);
};

#endif // TrackingNodeEDITOR_H_DEFINED
