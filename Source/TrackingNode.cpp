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
#include "TrackingNodeEditor.h"
#include "TrackingMessage.h"


// preallocate memory for msg
#define BUFFER_MSG_SIZE 256

std::ostream &
operator<<(std::ostream &stream, const TrackingModule &module)
{
    stream << "Address: " << module.m_address << std::endl;
    stream << "Port: " << module.m_port << std::endl;
    return stream;
}

/** ------------- Tracking Node Processor --------------- */

TrackingNode::TrackingNode()
    : GenericProcessor("Tracking Plugin")
    , m_isOn(true)
    , m_positionIsUpdated(false)
    , m_simulateTrajectory(false)
    , m_selectedCircle(-1)
    , m_selectedStimSource(-1)
    , m_timePassed(0.0)
    , m_currentTime(0.0)
    , m_previousTime(0.0)
    , m_timePassed_sim(0.0)
    , m_currentTime_sim(0.0)
    , m_previousTime_sim(0.0)
    , m_count(0)
    , m_forward(true)
    , m_rad(0.0)
    , m_outputChan(0)
    , m_pulseDuration(DEF_DUR)
    , m_ttlTriggered(false)
    , m_stimMode(stim_mode::uniform)
    , m_stimFreq(DEF_FREQ)
    , m_stimSD(DEF_SD)
    , messageReceived(false)
    , m_isRecordingTimeLogged(false)
    , m_isAcquisitionTimeLogged(false)
{
    addIntParameter(Parameter::GLOBAL_SCOPE, "Port", "Tracking source OSC port", DEF_PORT, 1024, 49151);

    addStringParameter(Parameter::GLOBAL_SCOPE, "Address", "Tracking source OSC address", DEF_ADDRESS);

    addCategoricalParameter(Parameter::GLOBAL_SCOPE, "Color", "Path color to be displayed",
                            colors,
                            0);

}

AudioProcessorEditor *TrackingNode::createEditor()
{
    editor = std::make_unique<TrackingNodeEditor>(this);
    return editor.get();
}

bool TrackingNode::addSource(String srcName, int port, String address, String color)
{
    if (port == 0)
    {
        auto nTrackers = trackers.size();
        if (nTrackers != 0)
        {
            std::vector<int> ports;
            for (int i = 0; i < nTrackers; ++i)
            {
                auto p = getPort(i);
                ports.push_back(p);
            }
            int maxPort = *std::max_element(ports.begin(), ports.end());
            port = maxPort + 1;
        }
        else
        {
            port = (int)getParameter("Port")->getValue();
        }
    }
    
    if (color.isEmpty())
        color = getParameter("Color")->getValueAsString();

    if (address.isEmpty())
        address = getParameter("Address")->getValueAsString();

    LOGD("Adding tacking module...");
    auto* tm = new TrackingModule(srcName, port, address, color, this);
    
    if(tm->m_server->isBoundAndRunning())
    {
        trackers.add(tm);
        LOGD("Added tracking module!");

        return true;
    }
    else
    {
        LOGD("Unable to bind to port: ", port);
        delete tm;
        return false;
    }
}

void TrackingNode::removeSource(int index)
{
    trackers.remove(index);
}

void TrackingNode::setPort (int i, int port)
{
    if (i < 0 || i >= trackers.size ())
    {
        return;
    }

    String address = trackers[i]->m_address;
    String color = trackers[i]->m_color;
    String name = trackers[i]->source.name;

    try
    {
        auto module = new TrackingModule(name, port, address, color, this);
        trackers.set(i, module, true);
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "Set port: " << e.what() << std::endl;
    }
}

int TrackingNode::getPort(int i)
{
	if (i < 0 || i >= trackers.size()) {
        LOGD("Invalid source index");
		return 0;
	}

    return trackers[i]->m_port;
}

void TrackingNode::setAddress (int i, String address)
{
    if (i < 0 || i >= trackers.size ())
    {
        LOGD("Invalid source index");
        return;
    }

    trackers[i]->m_address = address;
}

String TrackingNode::getAddress(int i)
{
    if (i < 0 || i >= trackers.size ()) {
		LOGD("Invalid source index");
        return String();
    }

    return trackers[i]->m_address;
}

void TrackingNode::setColor (int i, String color)
{
	if (i < 0 || i >= trackers.size())
	{
		LOGD("Invalid source index");
        return;
	}
	trackers[i]->m_color = color;
	trackers[i]->source.color = color;
}

String TrackingNode::getColor(int i)
{
	if (i < 0 || i >= trackers.size()) {
		LOGD("Invalid source index");
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

void TrackingNode::setSimulateTrajectory(bool sim)
{
    m_simulateTrajectory = sim;
}

std::vector<StimCircle> TrackingNode::getCircles()
{
    return m_circles;
}

void TrackingNode::addCircle(StimCircle c)
{
    m_circles.push_back(c);
}

void TrackingNode::editCircle(int ind, float x, float y, float rad, bool on)
{
    m_circles[ind].set(x,y,rad,on);
}

void TrackingNode::deleteCircle(int ind)
{
    if (m_circles.size())
        m_circles.erase(m_circles.begin() + ind);
}

void TrackingNode::disableCircles()
{
    for(int i=0; i<m_circles.size(); i++)
        m_circles[i].off();
}

int TrackingNode::getSelectedCircle() const
{
    return m_selectedCircle;
}

void TrackingNode::setSelectedCircle(int ind)
{
    m_selectedCircle = ind;
}

int TrackingNode::getSelectedStimSource() const
{
    return m_selectedStimSource;
}

void TrackingNode::setSelectedStimSource(int source)
{
    m_selectedStimSource = source;
}

int TrackingNode::getOutputChan() const
{
    return m_outputChan;
}

void TrackingNode::setOutputChan(int chan)
{
    m_outputChan = chan;
}

float TrackingNode::getStimFreq() const
{
    return m_stimFreq;
}

void TrackingNode::setStimFreq(float stimFreq)
{
    m_stimFreq = stimFreq;
}

float TrackingNode::getStimSD() const
{
    return m_stimSD;
}

void TrackingNode::setStimSD(float stimSD)
{
    m_stimSD = stimSD;
}

stim_mode TrackingNode::getStimMode() const
{
    return m_stimMode;
}

void TrackingNode::setStimMode(stim_mode mode)
{
    m_stimMode = mode;
}

int TrackingNode::getTTLDuration() const
{
    return m_pulseDuration;
}

void TrackingNode::setTTLDuration(int dur)
{
    m_pulseDuration = dur;
}

int TrackingNode::isPositionWithinCircles(float x, float y)
{
    int whichCircle = -1;
    for (int i = 0; i < m_circles.size() && whichCircle == -1; i++)
    {
        if (m_circles[i].isPositionIn(x,y) && m_circles[i].getOn())
            whichCircle = i;
    }
    return whichCircle;
}


void TrackingNode::parameterValueChanged(Parameter *param)
{
    auto trackingEditor = (TrackingNodeEditor*)getEditor();

    if (param->getName().equalsIgnoreCase("Port"))
    {
        int port = static_cast<IntParameter*>(param)->getIntValue();
        setPort(trackingEditor->getSelectedSource(), port);
    }
    else if(param->getName().equalsIgnoreCase("Address"))
    {
        String address = param->getValueAsString();
        setAddress(trackingEditor->getSelectedSource(), address);
    }
    else if(param->getName().equalsIgnoreCase("Color"))
    {
        int colorIndex = static_cast<CategoricalParameter*>(param)->getSelectedIndex();
        setColor(trackingEditor->getSelectedSource(), colors[colorIndex]);
    }
}

void TrackingNode::updateSettings()
{
    settings.update(getDataStreams());

    for(auto stream : getDataStreams())
    {        
        EventChannel* ttlChan;
        EventChannel::Settings ttlChanSettings{
            EventChannel::Type::TTL,
            "Tracking stimulation output",
            "Triggers whenever the tracking postion enters a ROI",
            "tracking.event",
            getDataStream(stream->getStreamId())
        };

        ttlChan = new EventChannel(ttlChanSettings);

        eventChannels.add(ttlChan);
        eventChannels.getLast()->addProcessor(processorInfo.get());
        settings[stream->getStreamId()]->eventChannelPtr = eventChannels.getLast();
    }

}

void TrackingNode::triggerEvent()
{   
    for(auto stream : getDataStreams())
    {     
        int64 startSampleNum = getFirstSampleNumberForBlock(stream->getStreamId());
        int nSamples = getNumSamplesInBlock(stream->getStreamId());
        auto settingsModule = settings[stream->getStreamId()];

        // Create and Send ON event
        TTLEventPtr event = TTLEvent::createTTLEvent(settingsModule->eventChannelPtr, 
                                                     startSampleNum,
                                                     m_outputChan,
                                                     true);
        addEvent(event, 0);


        // Create OFF event
        int eventDurationSamp = static_cast<int>(ceil(m_pulseDuration / 1000.0f * stream->getSampleRate()));
        TTLEventPtr eventOff = TTLEvent::createTTLEvent(settingsModule->eventChannelPtr,
                                                        startSampleNum + eventDurationSamp,
                                                        m_outputChan,
                                                        false);

        // Add or schedule turning-off event
        // We don't care whether there are other turning-offs scheduled to occur either in
        // this buffer or later. The abilities to change event duration during acquisition and for
        // events to be longer than the timeout period create a lot of possibilities and edge cases,
        // but overwriting turnoffEvent unconditionally guarantees that this and all previously
        // turned-on events will be turned off by this "turning-off" if they're not already off.
        if(eventDurationSamp < nSamples)
            addEvent(eventOff, eventDurationSamp);
        else
            settingsModule->turnoffEvent = eventOff;
    }
}

void TrackingNode::process(AudioBuffer<float> &buffer)
{
    // turn off event from previous buffer if necessary
    for (auto stream : getDataStreams())
    {
        auto settingsModule = settings[stream->getStreamId()];

        if(!settingsModule->turnoffEvent)
            continue;

        int startSampleNum = getFirstSampleNumberForBlock(stream->getStreamId());
        int nSamples = getNumSamplesInBlock(settingsModule->turnoffEvent->getStreamId());
        int turnoffOffset = jmax(0, (int)(settingsModule->turnoffEvent->getSampleNumber() - startSampleNum));

        if(turnoffOffset < nSamples)
        {
            addEvent(settingsModule->turnoffEvent, turnoffOffset);
            settingsModule->turnoffEvent = nullptr;
        }
    }

    // Return if no message received
    if (!messageReceived)
    {
        return;
    }

    lock.enter();

    for (int i = 0; i < trackers.size(); ++i)
    {
        auto *message = trackers[i]->m_messageQueue->pop();
        if (!message)
            continue;
        
        trackers[i]->source.x_pos = message->position.x;
        trackers[i]->source.y_pos = message->position.y;
        trackers[i]->source.width = message->position.width;
        trackers[i]->source.height = message->position.height;

		//std::cout << "Adding position: " << trackers[i]->source.x_pos << ", " << trackers[i]->source.y_pos << std::endl;
    }

    m_positionIsUpdated = true;

    if (m_isOn && m_selectedStimSource != -1)
    {

        m_currentTime = Time::currentTimeMillis();
        m_timePassed = float(m_currentTime - m_previousTime) / 1000.f; // in seconds

        float xPos = trackers[m_selectedStimSource]->source.x_pos;
        float yPos = trackers[m_selectedStimSource]->source.y_pos;

        // Check if current position is within stimulation areas
        int circleIn = isPositionWithinCircles(xPos, yPos);

        if (circleIn != -1)
        {
            trackers[m_selectedStimSource]->source.positionInsideACircle = true;

            // Check if timePassed >= latency
            if (m_stimMode == ttl)
            {
                if (!m_ttlTriggered)
                {
                    triggerEvent();
                    m_ttlTriggered = true;
                }
            }
            else
            {
                float stim_interval;
                if (m_stimMode == uniform)  {
                    stim_interval = float(1.f / m_stimFreq);
                }
                else if (m_stimMode == gauss)                     //gaussian
                {
                    float dist_norm = m_circles[circleIn].distanceFromCenter(xPos, yPos) / m_circles[circleIn].getRad();
                    float k = -1.0 / std::log(m_stimSD);
                    float freq_gauss = m_stimFreq*std::exp(-pow(dist_norm,2)/k);
                    stim_interval = float(1/freq_gauss);
                }

                float stimulationProbability = m_timePassed / stim_interval;
                std::uniform_real_distribution<float> distribution(0.0, 1.0);
                float randomNumber = distribution(generator);

                if (stimulationProbability > 1)
                    std::cout << "WARNING: The tracking stimulation frequency is higher than the sampling frequency." << std::endl;

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

    lock.exit();
    messageReceived = false;
}

void TrackingNode::receiveMessage(int port, String address, const TrackingData &message)
{

    //std::cout << "TrackingNode processing receivedMessage" << std::endl;

    lock.enter();
    for (int i = 0; i < trackers.size(); ++i) 
    {
        if (trackers[i]->m_port != port || trackers[i]->m_address.compare(address) != 0)
            continue;

        if (CoreServices::getAcquisitionStatus())
        {
            if (!m_isAcquisitionTimeLogged)
            {
                m_startingAcqTimeMillis = Time::currentTimeMillis();
                m_isAcquisitionTimeLogged = true;
                //std::cout << "Starting Acquisition at Ts: " << m_startingAcqTimeMillis << std::endl;
                trackers[i]->m_messageQueue->clear();
                CoreServices::sendStatusMessage("Clearing queue before start acquisition");
            }
            // LOGC("m_positionIsUpdated: ", m_positionIsUpdated);
            // m_positionIsUpdated = true;

			//std::cout << "Acquisition is active, pushing message to queue" << std::endl;

            int64 ts = CoreServices::getSoftwareTimestamp();

            TrackingData outputMessage = message;
            outputMessage.timestamp = ts;
            trackers[i]->m_messageQueue->push(outputMessage);
            messageReceived = true;
        }
        else
            m_isAcquisitionTimeLogged = false;
    }
    lock.exit();
}

TrackingSources& TrackingNode::getTrackingSource(int i)
{
    if (i >= 0 && i < trackers.size())
        return trackers[i]->source;
}

void TrackingNode::clearPositionUpdated()
{
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
	((TrackingNodeEditor*)getEditor())->enable();
    return true;
}

bool TrackingNode::stopAcquisition()
{
    ((TrackingNodeEditor*)getEditor())->disable();
    return true;
}


// Class TrackingQueue methods
TrackingQueue::TrackingQueue()
    : m_head(-1), m_tail(-1)
{
    memset(m_buffer, 0, BUFFER_SIZE);
}

TrackingQueue::~TrackingQueue() {}

void TrackingQueue::push(const TrackingData &message)
{
    m_head = (m_head + 1) % BUFFER_SIZE;
    m_buffer[m_head] = message;
    ++_count;
}

TrackingData *TrackingQueue::pop()
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

int TrackingQueue::count() {
    return _count;
}


// Class TrackingServer methods
TrackingServer::TrackingServer()
    : Thread("OscListener Thread"), m_incomingPort(0), m_address("")
{
    
}

TrackingServer::TrackingServer(int port, String address, TrackingNode *processor)
    : Thread("OscListener Thread"), m_incomingPort(port), m_address(address), m_processor(processor)
{
    LOGC("Creating OSC server on port ", port, " with address ", address);

    try
    {
        m_listeningSocket = std::make_unique<UdpListeningReceiveSocket>(IpEndpointName("localhost", m_incomingPort), this);
        CoreServices::sendStatusMessage("OSC Server started!");
    }
    catch (const std::exception &e)
    {
        CoreServices::sendStatusMessage("OSC Server failed to start!");
        LOGC("Exception in creating TrackingServer(): ", String(e.what()));
    }
}

TrackingServer::~TrackingServer()
{
    // stop the OSC Listener thread running
    stop();
    stopThread(-1);
    waitForThreadToExit(-1);
}

void TrackingServer::ProcessMessage(const osc::ReceivedMessage &receivedMessage,
                                    const IpEndpointName &)
{
	 //std::cout << "Received message: " << receivedMessage.AddressPattern() << std::endl;
     
    try
    {
        uint32 argumentCount = 4;

        if (receivedMessage.ArgumentCount() != argumentCount)
        {
            LOGC("ERROR: TrackingServer received message with wrong number of arguments. ",
                "Expected ", argumentCount, ", got ", receivedMessage.ArgumentCount());
            return;
        }

        for (uint32 i = 0; i < receivedMessage.ArgumentCount(); i++)
        {
            if (receivedMessage.TypeTags()[i] != 'f')
            {
                LOGC("TrackingServer only support 'f' (floats), not '", String(receivedMessage.TypeTags()[i]));
                return;
            }
        }

        osc::ReceivedMessageArgumentStream args = receivedMessage.ArgumentStream();

        TrackingData trackingData;

        

        // Arguments:
        args >> trackingData.position.x;      // 0 - x
        args >> trackingData.position.y;      // 1 - y
        args >> trackingData.position.width;  // 2 - box width
        args >> trackingData.position.height; // 3 - box height
        args >> osc::EndMessage;

        //std::cout << "Message contents: " << trackingData.position.x << ", " <<
        //    trackingData.position.y << ", " <<
        //    trackingData.position.width << ", " <<
         //   trackingData.position.height << std::endl;
            

        if (std::strcmp(receivedMessage.AddressPattern(), m_address.toStdString().c_str()) != 0)
        {
            //LOGC("Address pattern mismatch; got ", receivedMessage.AddressPattern(), " expected ", m_address.toStdString().c_str());
            return;
        }
        // add trackingmodule to receive message call: processor->receiveMessage (m_incomingPort, m_address, trackingData);
        m_processor->receiveMessage(m_incomingPort, m_address, trackingData);
    }
    catch (osc::Exception &e)
    {
        // any parsing errors such as unexpected argument types, or
        // missing arguments get thrown as exceptions.
        LOGC("error while parsing message: ", String(receivedMessage.AddressPattern()), ": ", String(e.what()));
    }
}

void TrackingServer::run()
{
    sleep(1000);
    if(m_listeningSocket != nullptr)
        m_listeningSocket->Run();
}

void TrackingServer::stop()
{
    // Stop the oscpack OSC Listener Thread
    if (!isThreadRunning())
    {
        return;
    }

    if(m_listeningSocket != nullptr)
        m_listeningSocket->AsynchronousBreak();
}

bool TrackingServer::isBoundAndRunning()
{
    if(m_listeningSocket != nullptr)
        return m_listeningSocket->IsBound();
    else
        return false;
}


// StimArea methods

StimArea::StimArea() :
    m_cx(0),
    m_cy(0),
    m_on(false)
{
}

StimArea::StimArea(float x, float y, bool on) :
    m_cx(x),
    m_cy(y),
    m_on(on)
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

void StimArea::setX(float x)
{
    m_cx = x;
}
void StimArea::setY(float y)
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
    : m_rad(0), StimArea(0, 0, false)
{
}

StimCircle::StimCircle(float x, float y, float rad, bool on) : StimArea(x, y, on)
{
    m_rad = rad;
}

float StimCircle::getRad()
{
    return m_rad;
}

void StimCircle::setRad(float rad)
{
    m_rad = rad;
}
void StimCircle::set(float x, float y, float rad, bool on)
{
    m_cx = x;
    m_cy = y;
    m_rad = rad;
    m_on = on;
}

bool StimCircle::isPositionIn(float x, float y)
{
    if (pow(x - m_cx, 2) + pow(y - m_cy, 2)
        <= m_rad * m_rad)
        return true;
    else
        return false;
}

float StimCircle::distanceFromCenter(float x, float y) {
    return sqrt(pow(x - m_cx, 2) + pow(y - m_cy, 2));
}

String StimCircle::returnType()
{
    return String("circle");
}
