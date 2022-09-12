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

#ifndef TRACKINGNODEEDITOR_H
#define TRACKINGNODEEDITOR_H

#define MAX_SOURCES 10

#include <EditorHeaders.h>

class TrackingNodeEditor : public GenericEditor,
                           public ComboBox::Listener
{
public:
    TrackingNodeEditor(GenericProcessor *parentNode);
    virtual ~TrackingNodeEditor();

    // virtual void labelTextChanged (Label* labelThatHasChanged) override;
    void buttonEvent(Button *button);
    virtual void comboBoxChanged(ComboBox *c) override;

    virtual void updateSettings();
    // void updateLabels();

private:
    int selectedSource;

    void addTrackingSource();
    void removeTrackingSource();

    ScopedPointer<Label> positionLabel;
    ScopedPointer<Label> labelPort;
    ScopedPointer<Label> portLabel;
    ScopedPointer<Label> labelAdr;
    ScopedPointer<Label> adrLabel;
    ScopedPointer<Label> labelColor;
    ScopedPointer<Label> colorLabel;
    ScopedPointer<ComboBox> colorSelector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingNodeEditor);
};

class SourceSelectorControl : public ParameterEditor,
                              public ComboBox::Listener,
                              public Button::Listener
{
public:
    SourceSelectorControl(Parameter *);
    ~SourceSelectorControl() {}
    /** Respond to combo box selection */
    void comboBoxChanged(ComboBox *cb);
    /** Respond to button clicks */
    void buttonClicked(Button *button);
    void updateView() override;

private:
    std::unique_ptr<ComboBox> sourceSelector;
    std::unique_ptr<UtilityButton> plusButton;
    std::unique_ptr<UtilityButton> minusButton;
    Font titleFont = Font("CP Mono", "Plain", 14);
};
#endif // TRACKINGNODEEDITOR_H