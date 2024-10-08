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

//preallocate memory for msg
#define BUFFER_MSG_SIZE 256

using namespace std;

TrackingNodeSettings::TrackingNodeSettings() :
    m_port(1),
    m_address("27020"),
    m_color("red"),
    m_messageQueue(new TrackingQueue()),
    m_server(new TrackingServer(1,"27020"))
{
    m_server->addProcessor(new TrackingNode());
    m_server->startThread();
}

TrackingNode::TrackingNode() : GenericProcessor ("Tracking Port")
    , m_startingRecTimeMillis (0)
    , m_startingAcqTimeMillis (0)
    , m_positionIsUpdated (false)
    , m_isRecordingTimeLogged (false)
    , m_isAcquisitionTimeLogged (false)
    , m_received_msg (0)
{
    setProcessorType (Plugin::Processor::SOURCE);
    sendSampleCount = false;

    addCategoricalParameter(Parameter::GLOBAL_SCOPE,
        "Source",
        "Tracking source",
        { "Tracking source 1" },
        0);
    addSelectedChannelsParameter(Parameter::GLOBAL_SCOPE, "Port", "Tracking source OSC port", 1);
    addCategoricalParameter(Parameter::GLOBAL_SCOPE,
        "Color",
        "Tracking source color to be displayed",
        { "red",
        "green",
        "blue",
        "magenta",
        "cyan",
        "orange",
        "pink",
        "grey",
        "violet",
        "yellow" },
        0);
    addIntParameter(Parameter::GLOBAL_SCOPE, "Address", "Tracking source OSC address", 27020, 0, 32768);
    lastNumInputs = 0;
}

AudioProcessorEditor* TrackingNode::createEditor()
{
    auto editor = std::make_unique<AudioProcessorEditor>(this, true);
    return editor.get();
}

void TrackingNode::parameterValueChanged(Parameter* param) {
    if (param->getName().equalsIgnoreCase("Address")) {
        settings[param->getStreamId()]->m_address = (String)param->getValue();
    }
    else if (param->getName().equalsIgnoreCase("Port")) {
        settings[param->getStreamId()]->m_port = (int)param->getValue();
    }
    else if (param->getName().equalsIgnoreCase("color")) {
        settings[param->getStreamId()]->m_color = (String)param->getValue();
    }
}

//Since the data needs a maximum buffer size but the actual number of read bytes might be less, let's
//add that info as a metadata field.
void TrackingNode::updateSettings()
{
    cout << "Updating settings!" << endl;

    settings.update(getDataStreams());

    for (auto stream : getDataStreams()) {
        parameterValueChanged(stream->getParameter("Address"));
        parameterValueChanged(stream->getParameter("Color"));
        parameterValueChanged(stream->getParameter("Port"));

        EventChannel::Settings s{ EventChannel::Type::CUSTOM,
            "Tracking data",
            "Tracking data received from Bonsai. x, y, width, height",
            "external.tracking.rawData",
            getDataStream(stream->getStreamId())
        };

        eventChannels.add(new EventChannel(s));
        eventChannels.getLast()->addProcessor(processorInfo.get());
        settings[stream->getStreamId()]->eventChannel = eventChannels.getLast();
    }
    lastNumInputs = getNumInputs();
}

void TrackingNode::addSource (int port, String address, String color)
{
    cout << "Adding source" << port << endl;
    try
    {
        for (auto stream : getDataStreams()) {
            if ((*stream)["enable_stream"]) {
                auto module = TrackingNodeSettings();
                module.m_port = port;
                module.m_address = address;
                module.m_color = color;
            }
        }
        
        updateSettings();

    }
    catch (const std::runtime_error& e)
    {
        std::cout << "Add source: " << e.what() << std::endl;
    }

}

void TrackingNode::addSource ()
{
   /* cout << "Adding empty source" << endl;
    try
    {
        auto module = TrackingNodeSettings();
        TrackingNodeSettings.add (module);
    }
    catch (const std::runtime_error& e)
    {
        std::cout << "Add source: " << e.what() << std::endl;
    }*/

}

void TrackingNode::removeSource (int i)
{
   /* auto *current = TrackingNodeSettings.getReference(i);
    TrackingNodeSettings.remove(i);
    delete current;*/
}

bool TrackingNode::isPortUsed(int port)
{
    bool used = false;
    for (auto stream : getDataStreams()) {
        if ((*stream)["enable_stream"]) {
            TrackingNodeSettings* module = settings[stream->getStreamId()];
            if (module->m_port > -1)
                used = true;
        }
    }
    return used;
}

void TrackingNode::setPort (int i, int port)
{
    /*if (i < 0 || i >= TrackingNodeSettings.size ())
    {
        return;
    }

    auto *module = TrackingNodeSettings.getReference (i);
    module->m_port = port;
    String address = module->m_address;
    String color = module->m_color;
    if (address.compare("") != 0)
    {
        delete module;
        try
        {
            module = new TrackingNodeSettings(port, address, color, this);
			TrackingNodeSettings.set(i, module);
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "Set port: " << e.what() << std::endl;
        }
    }*/
}

int TrackingNode::getPort(int i)
{
	/*if (i < 0 || i >= TrackingNodeSettings.size()) {
		return -1;
	}

    auto *module = TrackingNodeSettings.getReference (i);
    return module->m_port;*/
}

void TrackingNode::setAddress (int i, String address)
{
    /*if (i < 0 || i >= TrackingNodeSettings.size ())
    {
        return;
    }

    auto *module = TrackingNodeSettings.getReference (i);
    module->m_address = address;
    int port = module->m_port;
    String color = module->m_color;
    if (port != -1)
    {
        delete module;
        try
        {
            module = new TrackingNodeSettings(port, address, color, this);
			TrackingNodeSettings.set(i, module);
        }
        catch (const std::runtime_error& e)
        {
            std::cout << "Set address: " << e.what() << std::endl;
        }
    }*/
}

String TrackingNode::getAddress(int i)
{
    /*if (i < 0 || i >= TrackingNodeSettings.size ()) {
		return String("");
    }

    auto *module = TrackingNodeSettings.getReference (i);
    return module->m_address;*/
}

void TrackingNode::setColor (int i, String color)
{
	/*if (i < 0 || i >= TrackingNodeSettings.size())
	{
		return;
	}
	auto *module = TrackingNodeSettings.getReference(i);
	module->m_color = color;
	TrackingNodeSettings.set(i, module);*/

}

String TrackingNode::getColor(int i)
{
	/*if (i < 0 || i >= TrackingNodeSettings.size()) {
		return String("");
	}
    
    auto *module = TrackingNodeSettings.getReference (i);
    return module->m_color;*/
}

void TrackingNode::process (AudioSampleBuffer&)
{
    checkForEvents();

    if (!m_positionIsUpdated)
    {
        return;
    }

    lock.enter();

    for (auto stream : getDataStreams()) {
        if ((*stream)["enable_stream"])
        {
            auto * module = settings[stream->getStreamId()];
            while (true) {
                auto* message = module->m_messageQueue->pop();
                if (!message) {
                    break;
                }

                const uint16 streamId = stream->getStreamId();
                const int64 firstSampleInBlock = getFirstSampleNumberForBlock(streamId);
                const uint32 numSamplesInBlock = getNumSamplesInBlock(streamId);

                setTimestampAndSamples(firstSampleInBlock,
                    uint64(message->timestamp),
                    numSamplesInBlock,
                    streamId);
                MetadataValueArray metadata;
                auto desc = MetadataDescriptor{ MetadataDescriptor::MetadataType::CHAR, 15, String("color"), String("Tracking source color to be displayed"), String("channelInfo.extra") };
                auto color = MetadataValue{ desc };
                color.setValue(module->m_color);
                metadata.add(color);
                desc = MetadataDescriptor{ MetadataDescriptor::MetadataType::INT32, 1, String("port"), String("Tracking source OSC port"), String("channelInfo.extra") };
                auto port = MetadataValue{ desc };
                port.setValue(module->m_port);
                metadata.add(port);
                desc = MetadataDescriptor{ MetadataDescriptor::MetadataType::CHAR, 15, String("address"), String("Tracking source OSC address"), String("channelInfo.extra") };
                auto address = MetadataValue{ desc };
                address.setValue(module->m_address.toLowerCase());
                metadata.add(address);

                const EventChannel* chan = settings[stream->getStreamId()]->eventChannel;
                BinaryEventPtr event = BinaryEvent::createBinaryEvent(chan,
                    message->timestamp,
                    reinterpret_cast<uint8_t*>(&(message->position)),
                    sizeof(TrackingPosition),
                    metadata);
                addEvent(event, firstSampleInBlock);
            }
        }
    }

    lock.exit();
    m_positionIsUpdated = false;

}

int TrackingNode::getTrackingNodeSettingsIndex(int port, String address)
{
    /*int index = -1;
    for (int i = 0; i < TrackingNodeSettings.size (); i++)
    {
        auto *current  = TrackingNodeSettings.getReference(i);
        if (current->m_port == port && current->m_address.compare(address) == 0)
            index = i;
    }
    return index;*/
}

int TrackingNode::getNSources()
{
    //return TrackingNodeSettings.size ();
}

void TrackingNode::receiveMessage (int port, String address, const TrackingData &message)
{
    //int index = getTrackingNodeSettingsIndex(port, address);
    //if (index != -1)
    //{
    //    auto *selectedModule = TrackingNodeSettings.getReference (index);

    //    lock.enter();

    //    if (CoreServices::getRecordingStatus())
    //    {
    //        if (!m_isRecordingTimeLogged)
    //        {
    //            m_received_msg = 0;
    //            m_startingRecTimeMillis =  Time::currentTimeMillis();
    //            m_isRecordingTimeLogged = true;
    //            std::cout << "Starting Recording Ts: " << m_startingRecTimeMillis << std::endl;
    //            selectedModule->m_messageQueue->clear();
    //            CoreServices::sendStatusMessage ("Clearing queue before start recording");
    //        }
    //    }
    //    else
    //    {
    //        m_isRecordingTimeLogged = false;
    //    }


    //    if (CoreServices::getAcquisitionStatus()) // && !CoreServices::getRecordingStatus())
    //    {
    //        if (!m_isAcquisitionTimeLogged)
    //        {
    //            m_startingAcqTimeMillis = Time::currentTimeMillis();
    //            m_isAcquisitionTimeLogged = true;
    //            std::cout << "Starting Acquisition at Ts: " << m_startingAcqTimeMillis << std::endl;
    //            selectedModule->m_messageQueue->clear();
    //            CoreServices::sendStatusMessage ("Clearing queue before start acquisition");
    //        }

    //        m_positionIsUpdated = true;

    //        // NOTE: We cannot trust the getGlobalTimestamp function because it can return
    //        // negative time deltas. The reason is unknown.
    //        int64 ts = CoreServices::getSoftwareTimestamp();

    //        TrackingData outputMessage = message;
    //        outputMessage.timestamp = ts;
    //        selectedModule->m_messageQueue->push (outputMessage);
    //        m_received_msg++;
    //    }
    //    else
    //        m_isAcquisitionTimeLogged = false;

    //    lock.exit();
    //}

}

bool TrackingNode::isReady()
{
    return true;
}

void TrackingNode::saveCustomParametersToXml (XmlElement* parentElement)
{
    /*XmlElement* mainNode = parentElement->createNewChildElement ("TrackingNode");

    for (int i = 0; i < TrackingNodeSettings.size(); i++)
    {
        auto *module = TrackingNodeSettings.getReference (i);
        XmlElement* source = new XmlElement("Source_"+String(i+1));
        source->setAttribute ("port", module->m_port);
        source->setAttribute ("address", module->m_address);
        source->setAttribute ("color", module->m_color);
        mainNode->addChildElement(source);
    }*/
}

void TrackingNode::loadCustomParametersFromXml (XmlElement* xml)
{
    /*if (xml == nullptr)
    {
        return;
    }

    for (auto* xmlNode : xml->getChildIterator()) {
        if (xmlNode->hasTagName("TrackingNode")) {
            for (auto* xml : xmlNode->getChildIterator()) {
                int port = xml->getIntAttribute("port");
                String address = xml->getStringAttribute("address");
                String color = xml->getStringAttribute("color");
                addSource(port, address, color);
            }
        }
    }*/
}

// Class TrackingQueue methods
TrackingQueue::TrackingQueue()
    : m_head (-1)
    , m_tail (-1)
{
    memset (m_buffer, 0, BUFFER_SIZE);
}

TrackingQueue::~TrackingQueue() {}

void TrackingQueue::push (const TrackingData &message)
{
    m_head = (m_head + 1) % BUFFER_SIZE;
    m_buffer[m_head] = message;
}

TrackingData* TrackingQueue::pop ()
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
TrackingServer::TrackingServer ()
    : Thread ("OscListener Thread")
    , m_incomingPort (0)
    , m_address ("")
{
}

TrackingServer::TrackingServer (int port, String address)
    : Thread ("OscListener Thread")
    , m_incomingPort (port)
    , m_address (address)
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

void TrackingServer::ProcessMessage (const osc::ReceivedMessage& receivedMessage,
                                     const IpEndpointName&)
{
    int64 ts = CoreServices::getGlobalTimestamp();
    try
    {
        uint32 argumentCount = 4;

        if ( receivedMessage.ArgumentCount() != argumentCount) {
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
        args >> trackingData.position.x; // 0 - x
        args >> trackingData.position.y; // 1 - y
        args >> trackingData.position.width; // 2 - box width
        args >> trackingData.position.height; // 3 - box height
        args >> osc::EndMessage;

        for (TrackingNode* processor : m_processors)
        {
            //            String address = processor->address();

            if ( std::strcmp ( receivedMessage.AddressPattern(), m_address.toStdString().c_str() ) != 0 )
            {
                continue;
            }
            // add TrackingNodeSettings to receive message call: processor->receiveMessage (m_incomingPort, m_address, trackingData);
            processor->receiveMessage (m_incomingPort, m_address, trackingData);
        }
    }
    catch ( osc::Exception& e )
    {
        // any parsing errors such as unexpected argument types, or
        // missing arguments get thrown as exceptions.
        DBG ("error while parsing message: " << receivedMessage.AddressPattern() << ": " << e.what() << "\n");
    }
}

void TrackingServer::addProcessor (TrackingNode* processor)
{
    m_processors.push_back (processor);
}

void TrackingServer::removeProcessor (TrackingNode* processor)
{
    m_processors.erase (std::remove (m_processors.begin(), m_processors.end(), processor), m_processors.end());
}

void TrackingServer::run()
{
    cout << "SLeeping!" << endl;
    sleep(1000);
    cout << "Running!" << endl;
    // Start the oscpack OSC Listener Thread
    try {
        m_listeningSocket = new UdpListeningReceiveSocket(IpEndpointName("localhost", m_incomingPort), this);
        sleep(1000);
        m_listeningSocket->Run();
    }
    catch (const std::exception& e)
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
