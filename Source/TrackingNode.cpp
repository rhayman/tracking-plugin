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

// bool TrackingNodeSettings::removeTracker(const String & moduleToRemove) {
//     for (int i = 0; i < trackers.size(); ++i) {
//         if (trackers[i]->m_name == moduleToRemove) {
//             auto idx = trackers.indexOf(trackers[i]);
//             trackers.remove(idx);
//             return true;
//         }
//     }
//     return false;
// }

// void TrackingNodeSettings::updateTracker(int idx, Parameter * param, juce::var value) {
//     if (param->getName().equalsIgnoreCase("Name"))
//         trackers[idx]->m_name = value.toString();
//     else if (param->getName().equalsIgnoreCase("Port"))
//         trackers[idx]->m_port = value.toString();
//     else if (param->getName().equalsIgnoreCase("Address"))
//         trackers[idx]->m_address = value.toString();
//     else if (param->getName().equalsIgnoreCase("Color"))
//         trackers[idx]->m_color = value.toString();
// }

// TTLEventPtr TrackingNodeSettings::createEvent(int idx, int64 sample_number)
// {
//     auto position = trackers[idx]->m_messageQueue->pop();
//     if (!position) {
//         LOGC("position message NULL");
//         return nullptr;
//     }
//     LOGC("got message");
//     Array<float> pos;
//     pos.add(position->position.x);
//     pos.add(position->position.y);
//     pos.add(position->position.height);
//     pos.add(position->position.width);
    
//     MetadataValuePtr p_pos = new MetadataValue(*desc_position);
//     MetadataValuePtr p_port = new MetadataValue(*desc_port);
//     MetadataValuePtr p_addr = new MetadataValue(*desc_address);
//     p_pos->setValue(pos);
//     p_port->setValue(trackers[idx]->m_port);
//     p_addr->setValue(trackers[idx]->m_address);
//     MetadataValueArray metadata;
//     metadata.add(p_pos);
//     metadata.add(p_port);
//     metadata.add(p_addr);
//     LOGC("Creating TTL event");
//     TTLEventPtr event = TTLEvent::createTTLEvent(trackers[idx]->eventChannel,
//                                                  sample_number,
//                                                  0,
//                                                  true,
//                                                  metadata);
//     LOGC("Created TTL event");
//     return event;
// };

std::ostream &
operator<<(std::ostream &stream, const TrackingModule &module)
{
    stream << "Address: " << module.m_address << std::endl;
    stream << "Port: " << module.m_port << std::endl;
    return stream;
}

TrackingNode::TrackingNode()
    : GenericProcessor("Tracking Plugin")
{
    addIntParameter(Parameter::GLOBAL_SCOPE, "Port", "Tracking source OSC port", DEF_PORT, 1024, 49151);

    addStringParameter(Parameter::GLOBAL_SCOPE, "Address", "Tracking source OSC address", DEF_ADDRESS);

    addCategoricalParameter(Parameter::GLOBAL_SCOPE, "Color", "Path color to be displayed",
                            colors,
                            0);

    m_positionIsUpdated = false;
    messageReceived = false;
    m_isRecordingTimeLogged = false;
    m_isAcquisitionTimeLogged = false;
}

AudioProcessorEditor *TrackingNode::createEditor()
{
    editor = std::make_unique<TrackingNodeEditor>(this);
    return editor.get();
}

// void TrackingNode::initialize(bool signalChainIsLoading) {
//     if ( getDataStreams().isEmpty() && !signalChainIsLoading ) {
        
//     }
// }

void TrackingNode::addSource(String srcName, int port, String address, String color)
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
            port = DEF_PORT;
        }
    }
    
    if (color.isEmpty())
        color = String(DEF_COLOR);

    if (address.isEmpty())
        address = String(DEF_ADDRESS);

    LOGC("adding module");
    auto tm = new TrackingModule(srcName, port, address, color, this);
    trackers.add(tm);
    LOGC("added module");

    // EventChannel *events;
    // EventChannel::Settings s{EventChannel::Type::TTL,
    //                         "Tracking data",
    //                         "Tracking data received from Bonsai. x, y, width, height",
    //                         "external.tracking.rawData",
    //                         getDataStream(stream->getStreamId)};
    // LOGC("creating event channel");
    // events = new EventChannel(s);
    // String id = "trackingsource";
    // events->setIdentifier(id);
    // events->addProcessor(processorInfo.get());
    // LOGC("added processor");
    // // add metadata
    // meta_name = new MetadataValue(*desc_name);
    // meta_name->setValue(moduleName);
    
    // meta_port = new MetadataValue(*desc_port);
    // meta_port->setValue(String(port));
    // meta_address = new MetadataValue(*desc_address);
    // meta_address->setValue(address);

    // // add some dummy pos data for now
    // Array<float> pos;
    // pos.add(-1);
    // pos.add(-1);
    // pos.add(-1);
    // pos.add(-1);
    // meta_position = new MetadataValue(*desc_position);
    // meta_position->setValue(pos);
    // events->addMetadata(desc_position.get(), meta_position);
    // events->addMetadata(desc_name.get(), meta_name);
    // events->addMetadata(desc_address.get(), meta_address);
    // events->addMetadata(desc_port.get(), meta_port);
    // events->addEventMetadata(*desc_position);
    // events->addEventMetadata(*desc_port);
    // events->addEventMetadata(*desc_address);
    // eventChannels.add(events);
    // tm->eventChannel = events;
    // settings[stream->getStreamId()]->trackers.add(tm);
    // CoreServices::updateSignalChain(getEditor());
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
		return 0;
	}

    return trackers[i]->m_port;
}

void TrackingNode::setAddress (int i, String address)
{
    if (i < 0 || i >= trackers.size ())
    {
        return;
    }

    trackers[i]->m_address = address;
}

String TrackingNode::getAddress(int i)
{
    if (i < 0 || i >= trackers.size ()) {
		return String();
    }

    return trackers[i]->m_address;
}

void TrackingNode::setColor (int i, String color)
{
	if (i < 0 || i >= trackers.size())
	{
		return;
	}
	trackers[i]->m_color = color;
	trackers[i]->source.color = color;
}

String TrackingNode::getColor(int i)
{
	if (i < 0 || i >= trackers.size()) {
		return String();
	}
    
    return trackers[i]->m_color;
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
    eventChannels.clear();

    parameterValueChanged(getParameter("Port"));
    parameterValueChanged(getParameter("Address"));
}

void TrackingNode::process(AudioBuffer<float> &buffer)
{
    if (!messageReceived)
    {
        return;
    }

    lock.enter();

    for (int i = 0; i < trackers.size(); ++i)
    {
        auto *message = trackers[i]->m_messageQueue->pop();
        if (!message)
            break;
        
        trackers[i]->source.x_pos = message->position.x;
        trackers[i]->source.y_pos = message->position.y;
        trackers[i]->source.width = message->position.width;
        trackers[i]->source.height = message->position.height;
    }

    m_positionIsUpdated = true;

    lock.exit();
    messageReceived = false;
}

void TrackingNode::receiveMessage(int port, String address, const TrackingData &message)
{

    lock.enter();
    for (int i = 0; i < trackers.size(); ++i) 
    {
        if (trackers[i]->m_port != port || trackers[i]->m_address.compare(address) != 0)
            continue;

        if (CoreServices::getRecordingStatus())
        {
            if (!m_isRecordingTimeLogged)
            {
                m_startingRecTimeMillis = Time::currentTimeMillis();
                m_isRecordingTimeLogged = true;
                std::cout << "Starting Recording Ts: " << m_startingRecTimeMillis << std::endl;
                trackers[i]->m_messageQueue->clear();
                CoreServices::sendStatusMessage("Clearing queue before start recording");
            }
        }
        else
        {
            m_isRecordingTimeLogged = false;
        }

        if (CoreServices::getAcquisitionStatus()) // && !CoreServices::getRecordingStatus())
        {
            if (!m_isAcquisitionTimeLogged)
            {
                m_startingAcqTimeMillis = Time::currentTimeMillis();
                m_isAcquisitionTimeLogged = true;
                std::cout << "Starting Acquisition at Ts: " << m_startingAcqTimeMillis << std::endl;
                trackers[i]->m_messageQueue->clear();
                CoreServices::sendStatusMessage("Clearing queue before start acquisition");
            }
            // LOGC("m_positionIsUpdated: ", m_positionIsUpdated);
            // m_positionIsUpdated = true;

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

// TODO: Both I/O methods need finishing
void TrackingNode::saveCustomParametersToXml(XmlElement *parentElement)
{
    // for (auto stream : getDataStreams())
    // {
    //     auto *moduleXml = parentElement->createNewChildElement("Tracking_Node");
    //     TrackingNodeSettings *module = settings[stream->getStreamId()];
    //     for (auto tracker : module->trackers) {
    //         moduleXml->setAttribute("Name", tracker->m_name);
    //         moduleXml->setAttribute("Port", tracker->m_port);
    //         moduleXml->setAttribute("Address", tracker->m_address);
    //     }
    // }
}

void TrackingNode::loadCustomParametersFromXml(XmlElement *xml)
{
    // for (auto *moduleXml : xml->getChildIterator())
    // {
    //     if (moduleXml->hasTagName("Tracking_Node"))
    //     {
    //         String name = moduleXml->getStringAttribute("Name", "Tracking source 1");
    //         String address = moduleXml->getStringAttribute("Address", "/red");
    //         String port = moduleXml->getStringAttribute("Port", "27020");

    //         addTracker(name, port, address);
    //     }
    // }
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
}

TrackingServer::~TrackingServer()
{
    // stop the OSC Listener thread running
    stop();
    stopThread(-1);
    waitForThreadToExit(-1);
    delete m_listeningSocket;
}

void TrackingServer::ProcessMessage(const osc::ReceivedMessage &receivedMessage,
                                    const IpEndpointName &)
{
    int64 ts = CoreServices::getGlobalTimestamp();
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

        if (std::strcmp(receivedMessage.AddressPattern(), m_address.toStdString().c_str()) != 0)
        {
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
    CoreServices::sendStatusMessage("Server running");
    // Start the oscpack OSC Listener Thread
    try
    {
        m_listeningSocket = new UdpListeningReceiveSocket(IpEndpointName("localhost", m_incomingPort), this);
        sleep(1000);
        m_listeningSocket->Run();
    }
    catch (const std::exception &e)
    {
        LOGC("Exception in TrackingServer::run(): ", String(e.what()));
    }
}

void TrackingServer::stop()
{
    // Stop the oscpack OSC Listener Thread
    if (!isThreadRunning())
    {
        return;
    }

    m_listeningSocket->AsynchronousBreak();
}
