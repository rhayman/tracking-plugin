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

#include "TrackingNode.h"
#include "TrackingMessage.h"
#include "TrackingNodeEditor.h"

// preallocate memory for msg
#define BUFFER_MSG_SIZE 256

std::ostream&
    operator<< (std::ostream& stream, const TrackingModule& module)
{
    stream << "Address: " << module.m_address << std::endl;
    stream << "Port: " << module.m_port << std::endl;
    return stream;
}

/** ------------- Tracking Node Processor --------------- */

TrackingNode::TrackingNode()
    : GenericProcessor ("Tracking Plugin"), m_isOn (true), m_positionIsUpdated (false), m_simulateTrajectory (false), m_selectedCircle (-1), m_selectedStimSource (-1), m_timePassed (0.0), m_currentTime (0.0), m_previousTime (0.0), m_timePassed_sim (0.0), m_currentTime_sim (0.0), m_previousTime_sim (0.0), m_count (0), m_forward (true), m_rad (0.0), m_outputChan (0), m_pulseDuration (DEF_DUR), m_ttlTriggered (false), m_stimMode (stim_mode::uniform), m_stimFreq (DEF_FREQ), m_stimSD (DEF_SD), messageReceived (false)
{
}

void TrackingNode::registerParameters()
{
    addBooleanParameter (Parameter::PROCESSOR_SCOPE, "StimOn", "Stim", "Toggle stimulation", true);
}

AudioProcessorEditor* TrackingNode::createEditor()
{
    editor = std::make_unique<TrackingNodeEditor> (this);
    return editor.get();
}

bool TrackingNode::addSource (String srcName, int port, String address, String color)
{
    auto trackingEditor = (TrackingNodeEditor*) getEditor();
    if (port == 0)
    {
        auto nTrackers = trackers.size();
        if (nTrackers != 0)
        {
            std::vector<int> ports;
            for (int i = 0; i < nTrackers; ++i)
            {
                auto p = getPort (i);
                ports.push_back (p);
            }
            int maxPort = *std::max_element (ports.begin(), ports.end());
            port = maxPort + 1;
        }
        else
        {
            port = trackingEditor->getPort();
        }
    }

    if (color.isEmpty())
        color = trackingEditor->getColor();

    if (address.isEmpty())
        address = trackingEditor->getAddress();

    LOGD ("Adding tacking module...");
    auto* tm = new TrackingModule (srcName, port, address, color, this);

    if (tm->isConnected)
    {
        trackers.add (tm);
        LOGD ("Added tracking module!");

        return true;
    }
    else
    {
        LOGD ("Unable to bind to port: ", port);
        delete tm;
        return false;
    }
}

void TrackingNode::removeSource (int index)
{
    trackers.remove (index);
}

void TrackingNode::setPort (int i, int port)
{
    if (i < 0 || i >= trackers.size())
    {
        return;
    }

    String address = trackers[i]->m_address;
    String color = trackers[i]->m_color;
    String name = trackers[i]->source.name;

    try
    {
        auto module = new TrackingModule (name, port, address, color, this);
        trackers.set (i, module, true);
        LOGC ("Set port to ", port, " for ", name);
    }
    catch (const std::runtime_error& e)
    {
        LOGE ("Set port: ", e.what());
    }
}

int TrackingNode::getPort (int i)
{
    if (i < 0 || i >= trackers.size())
    {
        LOGD ("Invalid source index");
        return 0;
    }

    return trackers[i]->m_port;
}

void TrackingNode::setAddress (int i, String address)
{
    if (i < 0 || i >= trackers.size())
    {
        LOGD ("Invalid source index");
        return;
    }

    int port = trackers[i]->m_port;
    String color = trackers[i]->m_color;
    String name = trackers[i]->source.name;

    try
    {
        auto module = new TrackingModule (name, port, address, color, this);
        trackers.set (i, module, true);
        LOGC ("Set address to ", address, " for ", trackers[i]->m_name);
    }
    catch (const std::runtime_error& e)
    {
        LOGE ("Set address: ", e.what());
    }
}

String TrackingNode::getAddress (int i)
{
    if (i < 0 || i >= trackers.size())
    {
        LOGD ("Invalid source index");
        return String();
    }

    return trackers[i]->m_address;
}

void TrackingNode::setColor (int i, String color)
{
    if (i < 0 || i >= trackers.size())
    {
        LOGD ("Invalid source index");
        return;
    }
    trackers[i]->m_color = color;
    trackers[i]->source.color = color;
}

String TrackingNode::getColor (int i)
{
    if (i < 0 || i >= trackers.size())
    {
        LOGD ("Invalid source index");
        return String();
    }

    return trackers[i]->m_color;
}

void TrackingNode::startStimulation()
{
    m_isOn = true;
}

void TrackingNode::stopStimulation()
{
    m_isOn = false;
}

bool TrackingNode::getSimulateTrajectory() const
{
    return m_simulateTrajectory;
}

void TrackingNode::setSimulateTrajectory (bool sim)
{
    m_simulateTrajectory = sim;
}

std::vector<StimCircle> TrackingNode::getCircles()
{
    return m_circles;
}

void TrackingNode::addCircle (StimCircle c)
{
    m_circles.push_back (c);
}

void TrackingNode::editCircle (int ind, float x, float y, float rad, bool on)
{
    m_circles[ind].set (x, y, rad, on);
}

void TrackingNode::deleteCircle (int ind)
{
    if (m_circles.size())
        m_circles.erase (m_circles.begin() + ind);
}

void TrackingNode::disableCircles()
{
    for (int i = 0; i < m_circles.size(); i++)
        m_circles[i].off();
}

int TrackingNode::getSelectedCircle() const
{
    return m_selectedCircle;
}

void TrackingNode::setSelectedCircle (int ind)
{
    m_selectedCircle = ind;
}

int TrackingNode::getSelectedStimSource() const
{
    return m_selectedStimSource;
}

void TrackingNode::setSelectedStimSource (int source)
{
    LOGD ("Setting selected stim source to ", source);
    m_selectedStimSource = source;
}

int TrackingNode::getOutputChan() const
{
    return m_outputChan;
}

void TrackingNode::setOutputChan (int chan)
{
    m_outputChan = chan;
}

float TrackingNode::getStimFreq() const
{
    return m_stimFreq;
}

void TrackingNode::setStimFreq (float stimFreq)
{
    m_stimFreq = stimFreq;
}

float TrackingNode::getStimSD() const
{
    return m_stimSD;
}

void TrackingNode::setStimSD (float stimSD)
{
    m_stimSD = stimSD;
}

stim_mode TrackingNode::getStimMode() const
{
    return m_stimMode;
}

void TrackingNode::setStimMode (stim_mode mode)
{
    m_stimMode = mode;
}

int TrackingNode::getTTLDuration() const
{
    return m_pulseDuration;
}

void TrackingNode::setTTLDuration (int dur)
{
    m_pulseDuration = dur;
}

int TrackingNode::isPositionWithinCircles (float x, float y)
{
    int whichCircle = -1;
    for (int i = 0; i < m_circles.size() && whichCircle == -1; i++)
    {
        if (m_circles[i].isPositionIn (x, y) && m_circles[i].getOn())
            whichCircle = i;
    }
    return whichCircle;
}

void TrackingNode::parameterValueChanged (Parameter* param)
{
    if (param->getName().equalsIgnoreCase ("StimOn"))
    {
        bool stimOn = static_cast<BooleanParameter*> (param)->getBoolValue();
        if (stimOn)
            startStimulation();
        else
            stopStimulation();
    }
}

void TrackingNode::updateSettings()
{
    settings.update (getDataStreams());

    for (auto stream : getDataStreams())
    {
        EventChannel* ttlChan;
        EventChannel::Settings ttlChanSettings {
            EventChannel::Type::TTL,
            "Tracking stimulation output",
            "Triggers whenever the tracking postion enters a ROI",
            "tracking.event",
            getDataStream (stream->getStreamId())
        };

        ttlChan = new EventChannel (ttlChanSettings);

        eventChannels.add (ttlChan);
        eventChannels.getLast()->addProcessor (this);
        settings[stream->getStreamId()]->eventChannelPtr = eventChannels.getLast();
    }
}

void TrackingNode::triggerEvent()
{
    for (auto stream : getDataStreams())
    {
        int64 startSampleNum = getFirstSampleNumberForBlock (stream->getStreamId());
        int nSamples = getNumSamplesInBlock (stream->getStreamId());
        auto settingsModule = settings[stream->getStreamId()];

        // Create and Send ON event
        TTLEventPtr event = TTLEvent::createTTLEvent (settingsModule->eventChannelPtr,
                                                      startSampleNum,
                                                      m_outputChan,
                                                      true);
        addEvent (event, 0);

        // Create OFF event
        int eventDurationSamp = static_cast<int> (ceil (m_pulseDuration / 1000.0f * stream->getSampleRate()));
        TTLEventPtr eventOff = TTLEvent::createTTLEvent (settingsModule->eventChannelPtr,
                                                         startSampleNum + eventDurationSamp,
                                                         m_outputChan,
                                                         false);

        // Add or schedule turning-off event
        // We don't care whether there are other turning-offs scheduled to occur either in
        // this buffer or later. The abilities to change event duration during acquisition and for
        // events to be longer than the timeout period create a lot of possibilities and edge cases,
        // but overwriting turnoffEvent unconditionally guarantees that this and all previously
        // turned-on events will be turned off by this "turning-off" if they're not already off.
        if (eventDurationSamp < nSamples)
            addEvent (eventOff, eventDurationSamp);
        else
            settingsModule->turnoffEvent = eventOff;
    }
}

void TrackingNode::process (AudioBuffer<float>& buffer)
{
    // turn off event from previous buffer if necessary
    for (auto stream : getDataStreams())
    {
        auto settingsModule = settings[stream->getStreamId()];

        if (! settingsModule->turnoffEvent)
            continue;

        int startSampleNum = getFirstSampleNumberForBlock (stream->getStreamId());
        int nSamples = getNumSamplesInBlock (settingsModule->turnoffEvent->getStreamId());
        int turnoffOffset = jmax (0, (int) (settingsModule->turnoffEvent->getSampleNumber() - startSampleNum));

        if (turnoffOffset < nSamples)
        {
            addEvent (settingsModule->turnoffEvent, turnoffOffset);
            settingsModule->turnoffEvent = nullptr;
        }
    }

    // Return if no message received
    if (! messageReceived)
    {
        return;
    }

    lock.enter();

    for (int i = 0; i < trackers.size(); ++i)
    {
        while (true)
        {
            auto* message = trackers[i]->m_messageQueue->pop();
            if (! message)
            {
                if (! trackers[i]->positionData.empty())
                {
                    trackers[i]->source.x_pos = trackers[i]->positionData.back().x;
                    trackers[i]->source.y_pos = trackers[i]->positionData.back().y;
                    trackers[i]->source.width = trackers[i]->positionData.back().width;
                    trackers[i]->source.height = trackers[i]->positionData.back().height;
                }
                break;
            }

            trackers[i]->positionData.push_back (message->position);
        }

        //std::cout << "Adding position: " << trackers[i]->source.x_pos << ", " << trackers[i]->source.y_pos << std::endl;
    }

    m_positionIsUpdated = true;
    lock.exit();

    if (m_isOn && m_selectedStimSource != -1)
    {
        m_currentTime = Time::currentTimeMillis();
        m_timePassed = float (m_currentTime - m_previousTime) / 1000.f; // in seconds

        float xPos = trackers[m_selectedStimSource]->source.x_pos;
        float yPos = trackers[m_selectedStimSource]->source.y_pos;

        // Check if current position is within stimulation areas
        int circleIn = isPositionWithinCircles (xPos, yPos);

        if (circleIn != -1)
        {
            trackers[m_selectedStimSource]->source.positionInsideACircle = true;

            // Check if timePassed >= latency
            if (m_stimMode == ttl)
            {
                if (! m_ttlTriggered)
                {
                    triggerEvent();
                    m_ttlTriggered = true;
                }
            }
            else
            {
                float stim_interval;
                if (m_stimMode == uniform)
                {
                    stim_interval = float (1.f / m_stimFreq);
                }
                else if (m_stimMode == gauss) //gaussian
                {
                    float dist_norm = m_circles[circleIn].distanceFromCenter (xPos, yPos) / m_circles[circleIn].getRad();
                    float k = -1.0 / std::log (m_stimSD);
                    float freq_gauss = m_stimFreq * std::exp (-pow (dist_norm, 2) / k);
                    stim_interval = float (1 / freq_gauss);
                }

                float stimulationProbability = m_timePassed / stim_interval;
                std::uniform_real_distribution<float> distribution (0.0, 1.0);
                float randomNumber = distribution (generator);

                if (stimulationProbability > 1)
                    LOGC ("WARNING: The tracking stimulation frequency is higher than the sampling frequency.");

                if (randomNumber < stimulationProbability)
                {
                    triggerEvent();
                }
            }
        }
        else
        {
            trackers[m_selectedStimSource]->source.positionInsideACircle = false;
            m_ttlTriggered = false;
        }

        m_previousTime = m_currentTime;
    }

    messageReceived = false;
}

void TrackingNode::receiveMessage (int port, String address, const TrackingData& message)
{
    //std::cout << "TrackingNode processing receivedMessage" << std::endl;

    lock.enter();
    for (int i = 0; i < trackers.size(); ++i)
    {
        if (trackers[i]->m_port != port || trackers[i]->m_address.compare (address) != 0)
            continue;

        if (CoreServices::getAcquisitionStatus())
        {
            int64 ts = CoreServices::getSystemTime();

            TrackingData outputMessage = message;
            outputMessage.timestamp = ts;
            trackers[i]->m_messageQueue->push (outputMessage);
            messageReceived = true;
        }
    }
    lock.exit();
}

TrackingSources& TrackingNode::getTrackingSource (int i)
{
    if (i >= 0 && i < trackers.size())
        return trackers[i]->source;
}

std::vector<TrackingPosition> TrackingNode::getTrackingPositions (int i)
{
    if (i >= 0 && i < trackers.size())
        return trackers[i]->positionData;
}

void TrackingNode::clearPositionUpdated()
{
    for (int i = 0; i < trackers.size(); ++i)
    {
        trackers[i]->positionData.clear();
    }
    m_positionIsUpdated = false;
}

bool TrackingNode::positionIsUpdated() const
{
    return m_positionIsUpdated;
}

int TrackingNode::getNumSources()
{
    return trackers.size();
}

bool TrackingNode::startAcquisition()
{
    for (int i = 0; i < trackers.size(); i++)
        trackers[i]->m_messageQueue->clear();

    LOGC ("Clearing tracking message queue(s) before starting acquisition");

    ((TrackingNodeEditor*) getEditor())->enable();
    return true;
}

bool TrackingNode::stopAcquisition()
{
    ((TrackingNodeEditor*) getEditor())->disable();
    return true;
}

// Class TrackingQueue methods
TrackingQueue::TrackingQueue()
    : m_head (-1), m_tail (-1)
{
    memset (m_buffer, 0, BUFFER_SIZE);
}

TrackingQueue::~TrackingQueue() {}

void TrackingQueue::push (const TrackingData& message)
{
    m_head = (m_head + 1) % BUFFER_SIZE;
    m_buffer[m_head] = message;
    ++_count;
}

TrackingData* TrackingQueue::pop()
{
    if (isEmpty())
        return nullptr;

    m_tail = (m_tail + 1) % BUFFER_SIZE;
    --_count;
    return &(m_buffer[m_tail]);
}

bool TrackingQueue::isEmpty()
{
    return m_head == m_tail;
}

void TrackingQueue::clear()
{
    m_tail = -1;
    m_head = -1;
}

int TrackingQueue::count()
{
    return _count;
}

TrackingModule::TrackingModule (String name, int port, String address, String color, TrackingNode* processor)
    : m_name (name), m_port (port), m_address (address), m_color (color), m_processor (processor)
{
    source.color = color;
    source.name = name;
    source.x_pos = -1;
    source.y_pos = -1;
    source.width = -1;
    source.height = -1;
    source.positionInsideACircle = false;
    m_messageQueue = std::make_unique<TrackingQueue>();

    if (! connect (port))
    {
        LOGE ("Failed to connect to port ", port);
        return;
    }

    LOGC ("Creating OSC server on port ", port, " with address ", address);

    isConnected = true;
    addListener (this, m_address);
}

void TrackingModule::oscMessageReceived (const OSCMessage& message)
{
    //std::cout << "Received message: " << message.getAddressPattern() << std::endl;

    if (message.getAddressPattern() == OSCAddressPattern (m_address))
    {
        uint32 argumentCount = 4;

        if (message.size() != argumentCount)
        {
            LOGE ("TrackingServer received message with wrong number of arguments. ",
                  "Expected ",
                  argumentCount,
                  ", got ",
                  message.size());

            return;
        }

        for (uint32 i = 0; i < message.size(); i++)
        {
            if (! message[i].isFloat32())
            {
                LOGC ("TrackingServer only support floats, not '", String (message[i].getType()));
                return;
            }
        }

        TrackingData trackingData;

        trackingData.position.x = message[0].getFloat32();
        trackingData.position.y = message[1].getFloat32();
        trackingData.position.width = message[2].getFloat32();
        trackingData.position.height = message[3].getFloat32();

        m_processor->receiveMessage (m_port, m_address, trackingData);
    }
}

// StimArea methods

StimArea::StimArea() : m_cx (0),
                       m_cy (0),
                       m_on (false)
{
}

StimArea::StimArea (float x, float y, bool on) : m_cx (x),
                                                 m_cy (y),
                                                 m_on (on)
{
}

float StimArea::getX()
{
    return m_cx;
}
float StimArea::getY()
{
    return m_cy;
}
bool StimArea::getOn()
{
    return m_on;
}

void StimArea::setX (float x)
{
    m_cx = x;
}
void StimArea::setY (float y)
{
    m_cy = y;
}

bool StimArea::on()
{
    m_on = true;
    return m_on;
}
bool StimArea::off()
{
    m_on = false;
    return m_on;
}

// Circle methods

StimCircle::StimCircle()
    : m_rad (0), StimArea (0, 0, false)
{
}

StimCircle::StimCircle (float x, float y, float rad, bool on) : StimArea (x, y, on)
{
    m_rad = rad;
}

float StimCircle::getRad()
{
    return m_rad;
}

void StimCircle::setRad (float rad)
{
    m_rad = rad;
}
void StimCircle::set (float x, float y, float rad, bool on)
{
    m_cx = x;
    m_cy = y;
    m_rad = rad;
    m_on = on;
}

bool StimCircle::isPositionIn (float x, float y)
{
    if (pow (x - m_cx, 2) + pow (y - m_cy, 2)
        <= m_rad * m_rad)
        return true;
    else
        return false;

    // check if the point is inside the circle
}

float StimCircle::distanceFromCenter (float x, float y)
{
    return sqrt (pow (x - m_cx, 2) + pow (y - m_cy, 2));
}

String StimCircle::returnType()
{
    return String ("circle");
}
