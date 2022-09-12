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

#include "TrackingNode.h"
#include "TrackingNodeEditor.h"
#include "TrackingMessage.h"

// preallocate memory for msg
#define BUFFER_MSG_SIZE 256

using namespace std;

TrackingNodeSettings::TrackingNodeSettings(String name, int port, String address, String color)
    : m_source_name(name), m_port(port), m_address(address), m_color(color)
{
    m_messageQueue = nullptr;
    m_server = nullptr;
}

TrackingNodeSettings::TrackingNodeSettings() : m_port(1),
                                               m_address("27020"),
                                               m_color("red"),
                                               m_messageQueue(new TrackingQueue()),
                                               m_server(new TrackingServer(1, "27020"))
{
    m_server->addProcessor(new TrackingNode());
    m_server->startThread();
}

TTLEventPtr TrackingNodeSettings::createEvent(int64 sample_number, bool state)
{
    auto message = m_messageQueue->pop();
    // attach metadata to the TTL Event as BinaryEvents aren't dealt with (yet?)
    // in GenericProcessor::checkForEvents()
    auto name = MetadataValue(MetadataDescriptor::CHAR, 32);
    name.setValue(m_source_name);
    auto port = MetadataValue(MetadataDescriptor::INT16, 1);
    port.setValue(m_port);
    auto address = MetadataValue(MetadataDescriptor::CHAR, 16);
    address.setValue(m_address);
    auto color = MetadataValue(MetadataDescriptor::CHAR, 16);
    color.setValue(m_color);
    m_metadata.clear();
    m_metadata.add(name);
    m_metadata.add(port);
    m_metadata.add(address);
    m_metadata.add(color);

    TTLEventPtr event = TTLEvent::createTTLEvent(eventChannel,
                                                 message->timestamp,
                                                 16,
                                                 true,
                                                 m_metadata);

    return event;
};

TrackingNode::TrackingNode()
    : GenericProcessor("Tracking Port"), m_startingRecTimeMillis(0), m_startingAcqTimeMillis(0), m_positionIsUpdated(false), m_isRecordingTimeLogged(false), m_isAcquisitionTimeLogged(false), m_received_msg(0)
{
    addCategoricalParameter(Parameter::STREAM_SCOPE, "Source", "Tracking source", {"Tracking source 1"}, 0);
    addStringParameter(Parameter::STREAM_SCOPE, "Port", "Tracking source OSC port", "27020");
    addStringParameter(Parameter::STREAM_SCOPE, "Address", "Tracking source OSC address", "/red");
    addCategoricalParameter(Parameter::STREAM_SCOPE, "Color", "Tracking source color to be displayed",
                            {"red",
                             "green",
                             "blue",
                             "magenta",
                             "cyan",
                             "orange",
                             "pink",
                             "grey",
                             "violet",
                             "yellow"},
                            0);
    lastNumInputs = 0;
    LOGD("Completed ctor of TrackingNode");
}

TrackingNode::~TrackingNode()
{
}

AudioProcessorEditor *TrackingNode::createEditor()
{
    auto editor = std::make_unique<TrackingNodeEditor>(this);
    LOGD("Added editor");
    return editor.get();
}

void TrackingNode::parameterValueChanged(Parameter *param)
{
    if (param->getName().equalsIgnoreCase("Address"))
    {
        settings[param->getStreamId()]->m_address = param->getValueAsString();
    }
    else if (param->getName().equalsIgnoreCase("Port"))
    {
        settings[param->getStreamId()]->m_port = (int)param->getValue();
    }
    else if (param->getName().equalsIgnoreCase("color"))
    {
        settings[param->getStreamId()]->m_color = param->getValueAsString();
    }
}

// Since the data needs a maximum buffer size but the actual number of read bytes might be less, let's
// add that info as a metadata field.
void TrackingNode::updateSettings()
{
    LOGD("Updating settings...");
    settings.update(getDataStreams());

    for (auto stream : getDataStreams())
    {
        parameterValueChanged(stream->getParameter("Address"));
        parameterValueChanged(stream->getParameter("Color"));
        parameterValueChanged(stream->getParameter("Port"));

        EventChannel::Settings s{EventChannel::Type::CUSTOM,
                                 "Tracking data",
                                 "Tracking data received from Bonsai. x, y, width, height",
                                 "external.tracking.rawData",
                                 getDataStream(stream->getStreamId())};

        eventChannels.add(new EventChannel(s));
        eventChannels.getLast()->addProcessor(processorInfo.get());
        settings[stream->getStreamId()]->eventChannel = eventChannels.getLast();
    }
    lastNumInputs = getNumInputs();
    LOGD("Completed updating settings");
}

void TrackingNode::addSource(uint16 currentStream)
{
    LOGD("Adding source");
    EventChannel::Settings s{EventChannel::Type::CUSTOM,
                                 "Tracking data",
                                 "Tracking data received from Bonsai. x, y, width, height",
                                 "external.tracking.rawData",
                                 getDataStream(currentStream)};
    eventChannels.add(new EventChannel(s));
    eventChannels.getLast()->addProcessor(processorInfo.get());
    LOGD("Added source");
}

void TrackingNode::removeSource(String name) {
    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            TrackingNodeSettings *module = settings[stream->getStreamId()];
            for ( auto ch: eventChannels ) {
                if (module->eventChannel == ch)
                    delete ch;
            }
        }
    }
}

bool TrackingNode::isPortUsed(int port)
{
    bool used = false;
    // for (int i = 0; i < trackingModules.size(); i++)
    // {
    //     auto *current = trackingModules.getReference(i);
    //     if (current->m_port == port)
    //         used = true;
    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            TrackingNodeSettings *module = settings[stream->getStreamId()];
            if (module->m_port > -1)
                used = true;
        }
    }
    return used;
}

void TrackingNode::process(AudioSampleBuffer &)
{
    LOGD("Process..");
    checkForEvents();
    if (!m_positionIsUpdated)
        return;

    lock.enter();

    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            TrackingNodeSettings *module = settings[stream->getStreamId()];
            const uint16 streamId = stream->getStreamId();
            const int64 firstSampleInBlock = getFirstSampleNumberForBlock(streamId);
            const uint32 numSamplesInBlock = getNumSamplesInBlock(streamId);

            TTLEventPtr ptr = module->createEvent(firstSampleInBlock, true);
            addEvent(ptr, firstSampleInBlock);
        }
    }
    lock.exit();
    m_positionIsUpdated = false;
    LOGD("Processed");
}

int TrackingNode::getNSources()
{
    int nStreams = 0;
    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
            ++nStreams;
    }
    return nStreams;
}

void TrackingNode::receiveMessage(int port, String address, const TrackingData &message)
{
    LOGD("Receiving message");
    for (auto stream : getDataStreams())
    {
        if ((*stream)["enable_stream"])
        {
            TrackingNodeSettings *selectedModule = settings[stream->getStreamId()];
            if (CoreServices::getRecordingStatus())
            {
                if (!m_isRecordingTimeLogged)
                {
                    m_received_msg = 0;
                    m_startingRecTimeMillis = Time::currentTimeMillis();
                    m_isRecordingTimeLogged = true;
                    std::cout << "Starting Recording Ts: " << m_startingRecTimeMillis << std::endl;
                    selectedModule->m_messageQueue->clear();
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
                    selectedModule->m_messageQueue->clear();
                    CoreServices::sendStatusMessage("Clearing queue before start acquisition");
                }
                int64 ts = CoreServices::getSoftwareTimestamp();

                TrackingData outputMessage = message;
                outputMessage.timestamp = ts;
                selectedModule->m_messageQueue->push(outputMessage);
                m_received_msg++;
            }
            else
                m_isAcquisitionTimeLogged = false;

            lock.exit();
        }
    }
    LOGD("Received message");
}

bool TrackingNode::isReady()
{
    return true;
}

void TrackingNode::saveCustomParametersToXml(XmlElement *parentElement)
{
    for (auto stream : getDataStreams()) {
        XmlElement *mainNode = parentElement->createNewChildElement("TrackingNode");
        auto module = settings[stream->getStreamId()];
        XmlElement *source = new XmlElement(module->m_source_name);
        source->setAttribute("port", module->m_port);
        source->setAttribute("address", module->m_address);
        source->setAttribute("color", module->m_color);
    }
}

void TrackingNode::loadCustomParametersFromXml(XmlElement *xml)
{
    LOGD("Loading params");
    int streamIndex = 0;
    Array<const DataStream*> availableStreams = getDataStreams();
    for (auto* streamParams : xml->getChildIterator())
    {
        if (streamParams->hasTagName("TrackingNode"))
        {
        }
    }
    LOGD("Loaded params");
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
}

TrackingData *TrackingQueue::pop()
{
    if (isEmpty())
        return nullptr;

    m_tail = (m_tail + 1) % BUFFER_SIZE;
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

// Class TrackingServer methods
TrackingServer::TrackingServer()
    : Thread("OscListener Thread"), m_incomingPort(0), m_address("")
{
}

TrackingServer::TrackingServer(int port, String address)
    : Thread("OscListener Thread"), m_incomingPort(port), m_address(address)
{
}

TrackingServer::~TrackingServer()
{
    cout << "Destructing tracking server" << endl;
    // stop the OSC Listener thread running
    //    m_listeningSocket->Break();
    // allow the thread 2 seconds to stop cleanly - should be plenty of time.
    cout << "Destructed tracking server" << endl;
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
            cout << "ERROR: TrackingServer received message with wrong number of arguments. "
                 << "Expected " << argumentCount << ", got " << receivedMessage.ArgumentCount() << endl;
            return;
        }

        for (uint32 i = 0; i < receivedMessage.ArgumentCount(); i++)
        {
            if (receivedMessage.TypeTags()[i] != 'f')
            {
                cout << "TrackingServer only support 'f' (floats), not '"
                     << receivedMessage.TypeTags()[i] << "'" << endl;
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

        for (TrackingNode *processor : m_processors)
        {
            //            String address = processor->address();

            if (std::strcmp(receivedMessage.AddressPattern(), m_address.toStdString().c_str()) != 0)
            {
                continue;
            }
            // add trackingmodule to receive message call: processor->receiveMessage (m_incomingPort, m_address, trackingData);
            processor->receiveMessage(m_incomingPort, m_address, trackingData);
        }
    }
    catch (osc::Exception &e)
    {
        // any parsing errors such as unexpected argument types, or
        // missing arguments get thrown as exceptions.
        DBG("error while parsing message: " << receivedMessage.AddressPattern() << ": " << e.what() << "\n");
    }
}

void TrackingServer::addProcessor(TrackingNode *processor)
{
    m_processors.push_back(processor);
}

void TrackingServer::removeProcessor(TrackingNode *processor)
{
    m_processors.erase(std::remove(m_processors.begin(), m_processors.end(), processor), m_processors.end());
}

void TrackingServer::run()
{
    cout << "SLeeping!" << endl;
    sleep(1000);
    cout << "Running!" << endl;
    // Start the oscpack OSC Listener Thread
    try
    {
        m_listeningSocket = new UdpListeningReceiveSocket(IpEndpointName("localhost", m_incomingPort), this);
        sleep(1000);
        m_listeningSocket->Run();
    }
    catch (const std::exception &e)
    {
        std::cout << "Exception in TrackingServer::run(): " << e.what() << std::endl;
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
