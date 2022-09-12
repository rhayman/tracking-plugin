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

#ifndef TRACKINGNODE_H
#define TRACKINGNODE_H

#include <ProcessorHeaders.h>
#include "../../../plugin-GUI/Source/Utils/Utils.h"
#include "TrackingMessage.h"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/ip/IpEndpointName.h"
#include "oscpack/osc/OscReceivedElements.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/ip/UdpSocket.h"

#include <stdio.h>
#include <queue>
#include <utility>

#define BUFFER_SIZE 4096
#define MAX_SOURCES 10
#define DEF_PORT 27020
#define DEF_ADDRESS "/red"
#define DEF_COLOR "red"

using namespace std;

/**
    This helper class allows stores input tracking data in a circular queue.
*/
class TrackingQueue
{
public:
    TrackingQueue();
    ~TrackingQueue();

    void push(const TrackingData &message);
    TrackingData *pop();

    bool isEmpty();
    void clear();

private:
    TrackingData m_buffer[BUFFER_SIZE];
    int m_head;
    int m_tail;
};

/**
    This helper class is an OSC server running its own thread to keep data transmission
    continuous.
*/

class TrackingNode;

class TrackingServer : public osc::OscPacketListener,
                       public Thread
{
public:
    TrackingServer();
    TrackingServer(int port, String address);
    ~TrackingServer();

    void run();
    void stop();

    void addProcessor(TrackingNode *processor);
    void removeProcessor(TrackingNode *processor);

protected:
    virtual void ProcessMessage(const osc::ReceivedMessage &m, const IpEndpointName &);

private:
    TrackingServer(TrackingServer const &);
    void operator=(TrackingServer const &);

    int m_incomingPort;
    String m_address;

    UdpListeningReceiveSocket *m_listeningSocket = nullptr;
    std::vector<TrackingNode *> m_processors;
};

class TrackingNodeSettings
{
public:
    TrackingNodeSettings();
    TrackingNodeSettings(String, int, String, String);
    ~TrackingNodeSettings() {};
    TTLEventPtr createEvent(int64 sample_number, bool state);

    String m_source_name;
    int m_port = -1;
    String m_address;
    String m_color;
    TrackingQueue *m_messageQueue = nullptr;
    TrackingServer *m_server = nullptr;

    EventChannel *eventChannel;
    MetadataValueArray m_metadata;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingNodeSettings);
};

/**
    This source processor allows you to pipe tracking data via OSC signals from Bonsai tracker.

    @see TrackingNodeEditor
*/
class TrackingNode : public GenericProcessor
{
public:
    /** The class constructor, used to initialize any members. */
    TrackingNode();
    /** The class destructor, used to deallocate memory */
    ~TrackingNode();

    AudioProcessorEditor *createEditor() override;
    void updateSettings() override;
    void parameterValueChanged(Parameter *param);
    void process(AudioSampleBuffer &) override;
    bool isReady();
    void saveCustomParametersToXml(XmlElement *parentElement) override;
    void loadCustomParametersFromXml(XmlElement *parentElement) override;

    void receiveMessage(int port, String address, const TrackingData &message);
    int getTrackingModuleIndex(String name, int port, String address);
    void addSource(int port, String address, String color, uint16 currentStream);
    void addSource(uint16 currentStream);
    void removeSource(String name);

    int getNSources();
    bool isPortUsed(int port);

private:

    int64 m_startingRecTimeMillis;
    int64 m_startingAcqTimeMillis;

    CriticalSection lock;

    bool m_positionIsUpdated;
    bool m_isRecordingTimeLogged;
    bool m_isAcquisitionTimeLogged;
    int m_received_msg;

    Array<const EventChannel *> moduleEventChannels;
    int lastNumInputs;

    StreamSettings<TrackingNodeSettings> settings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingNode);
};

#endif // TRACKINGNODE_H
