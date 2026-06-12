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

/** ------------- Tracking Node DataThread --------------- */

TrackingNode::TrackingNode (SourceNode* sn)
    : DataThread (sn),
      m_isOn (true),
      m_positionIsUpdated (false),
      m_simulateTrajectory (false),
      m_selectedCircle (-1),
      m_selectedStimSource (-1),
      m_timePassed (0.0),
      m_currentTime (0),
      m_previousTime (0),
      m_timePassed_sim (0.0),
      m_currentTime_sim (0),
      m_previousTime_sim (0),
      m_count (0),
      m_forward (true),
      m_rad (0.0),
      m_outputChan (0),
      m_pulseDuration (DEF_DUR),
      m_ttlTriggered (false),
      m_ttlPulseRemaining (0),
      m_stimMode (stim_mode::uniform),
      m_stimFreq (DEF_FREQ),
      m_stimSD (DEF_SD),
      messageReceived (false)
{
    memset (totalSamples, 0, sizeof (totalSamples));
}

void TrackingNode::registerParameters()
{
    addBooleanParameter (Parameter::PROCESSOR_SCOPE, "StimOn", "Stim", "Toggle stimulation", true);
}

std::unique_ptr<GenericEditor> TrackingNode::createEditor (SourceNode* sn)
{
    return std::make_unique<TrackingNodeEditor> (sn, this);
}

bool TrackingNode::foundInputSource()
{
    // OSC is always available — return true if at least one tracker is configured
    return trackers.size() > 0;
}

void TrackingNode::updateSettings (OwnedArray<ContinuousChannel>* continuousChannels,
                                   OwnedArray<EventChannel>* eventChannels,
                                   OwnedArray<SpikeChannel>* spikeChannels,
                                   OwnedArray<DataStream>* sourceStreams,
                                   OwnedArray<DeviceInfo>* devices,
                                   OwnedArray<ConfigurationObject>* configurationObjects)
{
    continuousChannels->clear();
    eventChannels->clear();
    spikeChannels->clear();
    sourceStreams->clear();
    devices->clear();
    configurationObjects->clear();
    sourceBuffers.clear();

    for (int i = 0; i < trackers.size(); ++i)
    {
        // One DataStream per tracking source, with 4 continuous channels (x, y, w, h)
        DataStream::Settings dsSettings {
            trackers[i]->source.name,
            "Tracking position data (x, y, width, height)",
            "tracking." + trackers[i]->source.name.toLowerCase().replace (" ", "_"),
            float (TRACKING_FREQ)
        };
        sourceStreams->add (new DataStream (dsSettings));
        DataStream* stream = sourceStreams->getLast();

        // Channel 0: x position
        ContinuousChannel::Settings xSettings {
            ContinuousChannel::Type::AUX,
            "x",
            "X position in frame pixels",
            "tracking.x",
            1.0f,
            stream
        };
        continuousChannels->add (new ContinuousChannel (xSettings));

        // Channel 1: y position
        ContinuousChannel::Settings ySettings {
            ContinuousChannel::Type::AUX,
            "y",
            "Y position in frame pixels",
            "tracking.y",
            1.0f,
            stream
        };
        continuousChannels->add (new ContinuousChannel (ySettings));

        // Channel 2: width
        ContinuousChannel::Settings wSettings {
            ContinuousChannel::Type::AUX,
            "width",
            "Frame width in pixels",
            "tracking.width",
            1.0f,
            stream
        };
        continuousChannels->add (new ContinuousChannel (wSettings));

        // Channel 3: height
        ContinuousChannel::Settings hSettings {
            ContinuousChannel::Type::AUX,
            "height",
            "Frame height in pixels",
            "tracking.height",
            1.0f,
            stream
        };
        continuousChannels->add (new ContinuousChannel (hSettings));

        // TTL event channel for stimulation output (one per source stream)
        EventChannel::Settings ttlSettings {
            EventChannel::Type::TTL,
            "Tracking stimulation output",
            "Triggers whenever the tracked position enters a stimulation ROI",
            "tracking.event",
            stream,
            8
        };
        eventChannels->add (new EventChannel (ttlSettings));

        // DataBuffer: 4 channels, large enough to hold ~10 s at TRACKING_FREQ
        sourceBuffers.add (new DataBuffer (4, TRACKING_FREQ * 10));
    }
}

bool TrackingNode::startAcquisition()
{
    for (int i = 0; i < trackers.size(); ++i)
        trackers[i]->m_messageQueue->clear();

    memset (totalSamples, 0, sizeof (totalSamples));
    m_ttlPulseRemaining = 0;
    m_ttlTriggered = false;
    m_previousTime = Time::currentTimeMillis();

    LOGC ("Clearing tracking message queue(s) before starting acquisition");

    startThread();
    return true;
}

bool TrackingNode::stopAcquisition()
{
    if (isThreadRunning())
        signalThreadShouldExit();

    stopThread (500);
    return true;
}

bool TrackingNode::updateBuffer()
{
    if (! messageReceived)
    {
        Thread::sleep (5); // avoid spinning; OSC data arrives at ~20 Hz
        return true;
    }

    const ScopedLock sl (lock);
    messageReceived = false;

    m_currentTime = Time::currentTimeMillis();
    m_timePassed = float (m_currentTime - m_previousTime) / 1000.f; // seconds

    for (int i = 0; i < trackers.size(); ++i)
    {
        while (true)
        {
            auto* msg = trackers[i]->m_messageQueue->pop();
            if (! msg)
                break;

            // Keep positionData for the visualiser canvas
            trackers[i]->positionData.push_back (msg->position);

            // Update the live source state
            trackers[i]->source.x_pos = msg->position.x;
            trackers[i]->source.y_pos = msg->position.y;
            trackers[i]->source.width = msg->position.width;
            trackers[i]->source.height = msg->position.height;

            // Determine TTL state for this sample
            uint64 ttlCode = 0;

            if (m_ttlPulseRemaining > 0)
            {
                ttlCode = uint64 (1) << m_outputChan;
                --m_ttlPulseRemaining;
            }
            else if (m_isOn && m_selectedStimSource == i)
            {
                int circleIn = isPositionWithinCircles (msg->position.x, msg->position.y);

                if (circleIn != -1)
                {
                    trackers[i]->source.positionInsideACircle = true;
                    bool shouldTrigger = false;

                    if (m_stimMode == stim_mode::ttl)
                    {
                        if (! m_ttlTriggered)
                        {
                            shouldTrigger = true;
                            m_ttlTriggered = true;
                        }
                    }
                    else
                    {
                        float stimInterval;
                        if (m_stimMode == stim_mode::uniform)
                        {
                            stimInterval = 1.f / m_stimFreq;
                        }
                        else // gauss
                        {
                            float distNorm = m_circles[circleIn].distanceFromCenter (msg->position.x, msg->position.y)
                                             / m_circles[circleIn].getRad();
                            float k = -1.0f / std::log (m_stimSD);
                            float freqGauss = m_stimFreq * std::exp (-pow (distNorm, 2) / k);
                            stimInterval = 1.f / freqGauss;
                        }

                        float prob = m_timePassed / stimInterval;
                        if (prob > 1.f)
                            LOGC ("WARNING: Tracking stimulation frequency exceeds sample rate.");

                        std::uniform_real_distribution<float> dist (0.0f, 1.0f);
                        if (dist (generator) < prob)
                            shouldTrigger = true;
                    }

                    if (shouldTrigger)
                    {
                        int pulseSamples = jmax (1, (int) (m_pulseDuration / 1000.0f * TRACKING_FREQ));
                        m_ttlPulseRemaining = pulseSamples - 1; // consume first sample below
                        ttlCode = uint64 (1) << m_outputChan;
                    }
                }
                else
                {
                    trackers[i]->source.positionInsideACircle = false;
                    m_ttlTriggered = false;
                }
            }

            // Write position + TTL state to DataBuffer if one exists for this source
            if (i < sourceBuffers.size())
            {
                const float scaledX = msg->position.x * msg->position.width;
                const float scaledY = msg->position.y * msg->position.height;
                float data[4] = {
                    scaledX,
                    scaledY,
                    msg->position.width,
                    msg->position.height
                };

                int64 sampleNum = totalSamples[i];
                double timestamp = double (msg->timestamp) / 1000.0; // ms → s

                sourceBuffers[i]->addToBuffer (data, &sampleNum, &timestamp, &ttlCode, 1);
                ++totalSamples[i];
            }
        }
    }

    m_positionIsUpdated = true;
    m_previousTime = m_currentTime;

    return true;
}

bool TrackingNode::addSource (String srcName, int port, String address, String color)
{
    auto trackingEditor = (TrackingNodeEditor*) sn->getEditor();
    if (port == 0)
    {
        auto nTrackers = trackers.size();
        if (nTrackers != 0)
        {
            std::vector<int> ports;
            for (int i = 0; i < nTrackers; ++i)
                ports.push_back (getPort (i));
            port = *std::max_element (ports.begin(), ports.end()) + 1;
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

    LOGD ("Adding tracking module...");
    auto* tm = new TrackingModule (srcName, port, address, color, this);

    if (tm->isConnected)
    {
        trackers.add (tm);
        LOGD ("Added tracking module!");
        CoreServices::updateSignalChain (sn->getEditor());
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
    CoreServices::updateSignalChain (sn->getEditor());
}

void TrackingNode::setPort (int i, int port)
{
    if (i < 0 || i >= trackers.size())
        return;

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
    for (int i = 0; i < (int) m_circles.size(); i++)
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
    for (int i = 0; i < (int) m_circles.size() && whichCircle == -1; i++)
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

void TrackingNode::receiveMessage (int port, String address, const TrackingData& message)
{
    const ScopedLock sl (lock);

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
}

TrackingSources& TrackingNode::getTrackingSource (int i)
{
    jassert (i >= 0 && i < trackers.size());
    return trackers[i]->source;
}

std::vector<TrackingPosition> TrackingNode::getTrackingPositions (int i)
{
    if (i >= 0 && i < trackers.size())
        return trackers[i]->positionData;
    return {};
}

void TrackingNode::clearPositionUpdated()
{
    for (int i = 0; i < trackers.size(); ++i)
        trackers[i]->positionData.clear();
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

StimArea::StimArea() : m_cx (0), m_cy (0), m_on (false) {}

StimArea::StimArea (float x, float y, bool on) : m_cx (x), m_cy (y), m_on (on) {}

float StimArea::getX() { return m_cx; }
float StimArea::getY() { return m_cy; }
bool StimArea::getOn() { return m_on; }
void StimArea::setX (float x) { m_cx = x; }
void StimArea::setY (float y) { m_cy = y; }

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

StimCircle::StimCircle() : m_rad (0), StimArea (0, 0, false) {}

StimCircle::StimCircle (float x, float y, float rad, bool on) : StimArea (x, y, on)
{
    m_rad = rad;
}

float StimCircle::getRad() { return m_rad; }
void StimCircle::setRad (float rad) { m_rad = rad; }

void StimCircle::set (float x, float y, float rad, bool on)
{
    m_cx = x;
    m_cy = y;
    m_rad = rad;
    m_on = on;
}

bool StimCircle::isPositionIn (float x, float y)
{
    return (pow (x - m_cx, 2) + pow (y - m_cy, 2)) <= m_rad * m_rad;
}

float StimCircle::distanceFromCenter (float x, float y)
{
    return sqrt (pow (x - m_cx, 2) + pow (y - m_cy, 2));
}

String StimCircle::returnType()
{
    return String ("circle");
}
