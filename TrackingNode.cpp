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

TrackingNode::TrackingNode()
    : GenericProcessor ("Trackerizer"),
      m_isOn (true),
      m_positionIsUpdated (false),
      m_hasPendingMessages (false),
      m_simulateTrajectory (false),
      m_selectedCircle (-1),
      m_selectedStimSource (-1),
      m_timePassed (0.0),
      m_timePassed_sim (0.0),
      m_currentTime_sim (0),
      m_previousTime_sim (0),
      m_count (0),
      m_forward (true),
      m_rad (0.0),
      m_outputChan (0),
      m_pulseDuration (DEF_DUR),
      m_ttlTriggered (false),
      m_ttlIsOn (false),
      m_ttlOnSample (0),
      m_stimMode (stim_mode::uniform),
      m_stimFreq (DEF_FREQ),
      m_stimSD (DEF_SD),
      m_sampleNumber (0),
      m_lastProcessTicks (0),
      m_samplesAccumulated (0.0)
{
    setProcessorType (Plugin::Processor::SOURCE);
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

bool TrackingNode::generatesTimestamps() const
{
    return true;
}

float TrackingNode::getDefaultSampleRate() const
{
    return (float) TRACKING_FREQ;
}

void TrackingNode::updateSettings()
{
    // As a SOURCE, create our own DataStream (one per plugin instance).
    DataStream::Settings streamSettings {
        "Tracking",
        "Position data received via OSC",
        "tracking.stream",
        (float) TRACKING_FREQ,
        true // generates_timestamps
    };
    dataStreams.add (new DataStream (streamSettings));
    dataStreams.getLast()->addProcessor (this);

    DataStream* stream = dataStreams.getLast();

    // Two continuous channels per tracker: X position and Y position (both scaled
    // to pixel units by multiplying the normalised (0-1) value by width / height).
    for (int i = 0; i < trackers.size(); ++i)
    {
        String trackerName = trackers[i]->m_name;

        ContinuousChannel::Settings xSettings {
            ContinuousChannel::Type::AUX,
            trackerName + " X",
            "X position scaled by frame width (pixels)",
            "tracking.position.x",
            1.0f, // bitVolts: 1 count == 1 pixel
            stream
        };
        continuousChannels.add (new ContinuousChannel (xSettings));
        continuousChannels.getLast()->addProcessor (this);

        ContinuousChannel::Settings ySettings {
            ContinuousChannel::Type::AUX,
            trackerName + " Y",
            "Y position scaled by frame height (pixels)",
            "tracking.position.y",
            1.0f,
            stream
        };
        continuousChannels.add (new ContinuousChannel (ySettings));
        continuousChannels.getLast()->addProcessor (this);
    }

    // TTL event channel for stimulation output.
    EventChannel::Settings ttlSettings {
        EventChannel::Type::TTL,
        "Tracking stimulation output",
        "Triggers whenever the tracked position enters a stimulation ROI",
        "tracking.event",
        stream,
        8
    };
    eventChannels.add (new EventChannel (ttlSettings));
    eventChannels.getLast()->addProcessor (this);
}

bool TrackingNode::startAcquisition()
{
    for (int i = 0; i < trackers.size(); ++i)
        trackers[i]->m_messageQueue->clear();

    m_hasPendingMessages = false;
    m_positionIsUpdated = false;
    m_ttlIsOn = false;
    m_ttlTriggered = false;

    // Reset continuous-output timing counters.
    m_sampleNumber = 0;
    m_lastProcessTicks = Time::getHighResolutionTicks();
    m_samplesAccumulated = 0.0;

    LOGC ("Clearing tracking message queue(s) before starting acquisition");
    ((TrackingNodeEditor*) getEditor())->enable();

    return true;
}

bool TrackingNode::stopAcquisition()
{
    ((TrackingNodeEditor*) getEditor())->disable();

    return true;
}

void TrackingNode::process (AudioBuffer<float>& continuousBuffer)
{
    checkForEvents();

    if (dataStreams.isEmpty())
        return;

    DataStream* stream = dataStreams[0];
    uint16 streamId = stream->getStreamId();
    float sampleRate = stream->getSampleRate();

    // ---------------------------------------------------------------
    // Determine how many samples to emit this callback using a
    // wall-clock accumulator so the long-run rate matches sampleRate.
    // ---------------------------------------------------------------
    int64 currentTicks = Time::getHighResolutionTicks();
    double elapsedSecs = Time::highResolutionTicksToSeconds (currentTicks - m_lastProcessTicks);
    m_lastProcessTicks = currentTicks;

    m_samplesAccumulated += elapsedSecs * sampleRate;
    int nSamples = (int) m_samplesAccumulated;
    m_samplesAccumulated -= nSamples;
    nSamples = jmax (0, jmin (nSamples, continuousBuffer.getNumSamples()));

    // Time window used by the stochastic stimulation probability.
    m_timePassed = float (elapsedSecs);

    // Register the sample count and timestamp for this block.
    setTimestampAndSamples (m_sampleNumber,
                            (double) m_sampleNumber / sampleRate,
                            (uint32) nSamples,
                            streamId);

    int64 firstSample = m_sampleNumber;
    m_sampleNumber += nSamples;

    // ---------------------------------------------------------------
    // Find the TTL event channel for this stream.
    // ---------------------------------------------------------------
    EventChannel* ttlChannel = nullptr;
    for (auto ch : eventChannels)
    {
        if (ch->getStreamId() == streamId)
        {
            ttlChannel = ch;
            break;
        }
    }

    // ---------------------------------------------------------------
    // Turn off any active TTL pulse once its duration has elapsed.
    // ---------------------------------------------------------------
    if (m_ttlIsOn && ttlChannel != nullptr && sampleRate > 0.0f && nSamples > 0)
    {
        int64 pulseSamples = (int64) (m_pulseDuration / 1000.0f * sampleRate);
        int64 offSample = m_ttlOnSample + pulseSamples;

        if (firstSample + nSamples > offSample)
        {
            int offOffset = (int) jmax ((int64) 0, offSample - firstSample);
            TTLEventPtr offEvent = TTLEvent::createTTLEvent (ttlChannel, offSample, m_outputChan, false);
            addEvent (offEvent, offOffset);
            m_ttlIsOn = false;
            m_ttlTriggered = false;
        }
    }

    // ---------------------------------------------------------------
    // Drain the OSC queue, update positions, handle stimulation,
    // then write the latest position into the continuous buffer.
    // ---------------------------------------------------------------
    const ScopedLock sl (lock);

    for (int i = 0; i < trackers.size(); ++i)
    {
        while (true)
        {
            auto* msg = trackers[i]->m_messageQueue->pop();

            if (! msg)
                break;

            m_positionIsUpdated = true;

            trackers[i]->positionData.push_back (msg->position);

            trackers[i]->source.x_pos = msg->position.x;
            trackers[i]->source.y_pos = msg->position.y;
            trackers[i]->source.width = msg->position.width;
            trackers[i]->source.height = msg->position.height;

            if (! m_ttlIsOn && m_isOn && m_selectedStimSource == i
                && ttlChannel != nullptr && nSamples > 0)
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
                            LOGC ("WARNING: Tracking stimulation frequency exceeds callback rate.");

                        std::uniform_real_distribution<float> dist (0.0f, 1.0f);
                        if (dist (generator) < prob)
                            shouldTrigger = true;
                    }

                    if (shouldTrigger)
                    {
                        TTLEventPtr onEvent = TTLEvent::createTTLEvent (ttlChannel, firstSample, m_outputChan, true);
                        addEvent (onEvent, 0);
                        m_ttlIsOn = true;
                        m_ttlOnSample = firstSample;
                    }
                }
                else
                {
                    trackers[i]->source.positionInsideACircle = false;
                    m_ttlTriggered = false;
                }
            }
        }

        // Write the most recently received (or initialised) position to the
        // continuous buffer, scaled to pixel units.  Hold-last-value until a
        // new OSC message arrives.  x_pos / y_pos are -1 before any data
        // is received; output 0.0f as a neutral sentinel in that case.
        if (nSamples > 0)
        {
            float xVal = (trackers[i]->source.x_pos >= 0.0f)
                             ? trackers[i]->source.x_pos * trackers[i]->source.width
                             : 0.0f;
            float yVal = (trackers[i]->source.y_pos >= 0.0f)
                             ? trackers[i]->source.y_pos * trackers[i]->source.height
                             : 0.0f;

            auto xGlobalIdx = getGlobalChannelIndex (streamId, i * 2);
            auto yGlobalIdx = getGlobalChannelIndex (streamId, i * 2 + 1);

            if (xGlobalIdx >= 0 && xGlobalIdx < continuousBuffer.getNumChannels())
            {
                float* xPtr = continuousBuffer.getWritePointer (xGlobalIdx);
                for (int s = 0; s < nSamples; ++s)
                    xPtr[s] = xVal;
            }

            if (yGlobalIdx >= 0 && yGlobalIdx < continuousBuffer.getNumChannels())
            {
                float* yPtr = continuousBuffer.getWritePointer (yGlobalIdx);
                for (int s = 0; s < nSamples; ++s)
                    yPtr[s] = yVal;
            }
        }
    }

    bool queuesEmpty = true;
    for (int i = 0; i < trackers.size(); ++i)
    {
        if (! trackers[i]->m_messageQueue->isEmpty())
        {
            queuesEmpty = false;
            break;
        }
    }

    m_hasPendingMessages = ! queuesEmpty;
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
        CoreServices::updateSignalChain (getEditor());
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
    CoreServices::updateSignalChain (getEditor());
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
            m_hasPendingMessages = true;
            // LOGC ("Received tracking message from port ", port, " at address ", address, " with timestamp ", ts);
            // LOGC ("x is ", message.position.x, " y is ", message.position.y, " width is ", message.position.width, " height is ", message.position.height);
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
