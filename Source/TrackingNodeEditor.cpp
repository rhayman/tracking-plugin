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

#include "TrackingNodeEditor.h"
#include "TrackingNode.h"

TrackingNodeEditor::TrackingNodeEditor(GenericProcessor *parentNode)
    : GenericEditor(parentNode), selectedSource(0)
{
    LOGD("Creating editor");
    TrackingNode *processor = (TrackingNode *)getProcessor();
    auto src_param = processor->getParameter("Source");
    addCustomParameterEditor(new SourceSelectorControl(src_param), 45, 30);
    addSelectedChannelsParameterEditor("Port", 10, 55);
    addTextBoxParameterEditor("Address", 10, 80);
    addComboBoxParameterEditor("Color", 10, 105);
    LOGD("Created editor");
}

TrackingNodeEditor::~TrackingNodeEditor()
{
}


void TrackingNodeEditor::updateSettings()
{
}

void TrackingNodeEditor::comboBoxChanged(ComboBox *c) {}

SourceSelectorControl::SourceSelectorControl(Parameter *param) : ParameterEditor(param)
{
    LOGD("Creating source selector");
    sourceSelector = std::make_unique<ComboBox>("Source");
    sourceSelector->setBounds(45, 30, 130, 20);
    sourceSelector->addListener(this);
    addAndMakeVisible(sourceSelector.get());

    plusButton = std::make_unique<UtilityButton>("+", titleFont);
    plusButton->addListener(this);
    plusButton->setRadius(3.0f);
    plusButton->setBounds(10, 30, 20, 20);
    addAndMakeVisible(plusButton.get());

    minusButton = make_unique<UtilityButton>("-", titleFont);
    minusButton->addListener(this);
    minusButton->setRadius(3.0f);
    minusButton->setBounds(190, 30, 20, 20);
    addAndMakeVisible(minusButton.get());
    LOGD("Created source selector");
}

void SourceSelectorControl::comboBoxChanged(ComboBox *cb)
{
    if (cb == sourceSelector.get())
    {
    }
}

void SourceSelectorControl::buttonClicked(Button *btn)
{
    if (btn == plusButton.get())
    {
        // add a tracking source
        
    }
    if (btn == minusButton.get())
    {
        // remove a tracking source
        auto source_name = sourceSelector->getText();
        auto *ed = (TrackingNodeEditor*)getParentComponent();
        TrackingNode *p = (TrackingNode*)ed->getProcessor();
        p->removeSource(source_name);
    }
}

void SourceSelectorControl::updateView()
{
}