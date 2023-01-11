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

#ifndef TRACKINGNODE_H
#define TRACKINGNODE_H

#include <ProcessorHeaders.h>
#include "TrackingMessage.h"

#include "oscpack/osc/OscOutboundPacketStream.h"
#include "oscpack/ip/IpEndpointName.h"
#include "oscpack/osc/OscReceivedElements.h"
#include "oscpack/osc/OscPacketListener.h"
#include "oscpack/ip/UdpSocket.h"

#include <stdio.h>
#include <queue>
#include <utility>
#include <random>

#define BUFFER_SIZE 4096
#define MAX_SOURCES 10
#define DEF_PORT 27020
#define DEF_ADDRESS "/red"
#define DEF_COLOR "red"

// Tracking Stimulation defines
#define DEF_FREQ 2
#define DEF_SD 0.5
#define DEF_DUR 50
#define TRACKING_FREQ 20
#define MAX_CIRCLES 9



typedef enum
{
	uniform,
	gauss,
	ttl
} stim_mode;

//	This helper class allows stores input tracking data in a circular queue.
class TrackingQueue
{
public:
	TrackingQueue();
	~TrackingQueue();

	void push(const TrackingData &message);
	TrackingData *pop();

	bool isEmpty();
	void clear();

	int count();

private:
	TrackingData m_buffer[BUFFER_SIZE];
	int m_head;
	int m_tail;
	int _count = 0;
};

//	This helper class is an OSC server running its own thread to keep data transmission
//	continuous.

class TrackingNode;

class TrackingServer : public osc::OscPacketListener,
					   public Thread
{
public:
	TrackingServer();
	TrackingServer(int port, String address, TrackingNode *processor);
	~TrackingServer();

	void run();
	void stop();

	bool isBoundAndRunning();

protected:
	virtual void ProcessMessage(const osc::ReceivedMessage &m, const IpEndpointName &);

private:
	TrackingServer(TrackingServer const &);
	void operator=(TrackingServer const &);

	int m_incomingPort;
	String m_address;

	std::unique_ptr<UdpListeningReceiveSocket> m_listeningSocket;
	TrackingNode* m_processor;
};

class TrackingModule
{
public:
	TrackingModule() {}
	TrackingModule(String name, int port, String address, String color, TrackingNode *processor)
		:m_name(name), m_port(port), m_address(address), m_color(color)
	{
		source.color = color;
		source.name = name;
		source.x_pos = -1;
		source.y_pos = -1;
		source.width = -1;
		source.height = -1;
		source.positionInsideACircle = false;
		m_messageQueue = std::make_unique<TrackingQueue>();
		m_server = std::make_unique<TrackingServer>(port, address, processor);
		// m_server->addProcessor(processor);
		m_server->startThread();
	}

	~TrackingModule() {}

	friend std::ostream &operator<<(std::ostream &, const TrackingModule &);

	String m_name;
	int m_port = DEF_PORT;
	String m_address = String(DEF_ADDRESS);
	String m_color = String(DEF_COLOR);

	std::unique_ptr<TrackingQueue> m_messageQueue;
	std::unique_ptr<TrackingServer> m_server;

	TrackingSources source;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingModule);
};


/**

  Class for Abstrac Stimulation Area

*/
class StimArea
{
public:
    StimArea();
    StimArea(float x, float y, bool on);

    float getX();
    float getY();
    bool getOn();
    void setX(float x);
    void setY(float y);

    bool on();
    bool off();

    virtual bool isPositionIn(float x, float y) = 0;
    virtual float distanceFromCenter(float x, float y) = 0;
    virtual String returnType() = 0;

protected:

    float m_cx;
    float m_cy;
    bool m_on;

};
/**

  Class for Stimulation Circles

*/
class StimCircle : public StimArea
{
public:
    StimCircle();
    StimCircle(float x, float y, float r, bool on);

    float getRad();

    void setRad(float rad);
    void set(float x, float y, float rad, bool on);

    bool isPositionIn(float x, float y);
    float distanceFromCenter(float x, float y);
    String returnType();

private:
    float m_rad;
};


/** Holds settings for one stream's event channel */
class TrackingNodeSettings
{
public:
    /** Constructor -- sets default values*/
    TrackingNodeSettings() : 
		eventChannelPtr(nullptr), turnoffEvent(nullptr) { }
	
	/** Destructor*/
	~TrackingNodeSettings() { }

    /** Parameters */
    EventChannel* eventChannelPtr;
    TTLEventPtr turnoffEvent; // holds a turnoff event that must be added in a later buffer
};


class TrackingNode : public GenericProcessor
{

public:
	/** The class constructor, used to initialize any members. */
	TrackingNode();

	/** The class destructor, used to deallocate memory */
	~TrackingNode() {}

	/** If the processor has a custom editor, this method must be defined to instantiate it. */
	AudioProcessorEditor *createEditor() override;

	// void initialize(bool signalChainIsLoading) override;

	bool startAcquisition() override;

	bool stopAcquisition() override;

	// Creates a tracking module and adds a tracking source
	bool addSource(String name, int port=0, String address="", String color="");

	void removeSource(int index);

	void parameterValueChanged(Parameter *param) override;

	/** Called every time the settings of an upstream plugin are changed.
		Allows the processor to handle variations in the channel configuration or any other parameter
		passed through signal chain. The processor can use this function to modify channel objects that
		will be passed to downstream plugins. */
	void updateSettings() override;

	/** Defines the functionality of the processor.
		The process method is called every time a new data buffer is available.
		Visualizer plugins typically use this method to send data to the canvas for display purposes */
	void process(AudioBuffer<float> &buffer) override;

	// receives a message from the osc server
	void receiveMessage(int port, String address, const TrackingData &message);

	// Setter-Getters

	int getPort(int idx);
	void setPort (int idx, int port);

	String getAddress(int idx);
	void setAddress (int i, String address);

	void setColor (int i, String color);
    String getColor(int i);

	int getNumSources();

	TrackingSources &getTrackingSource(int i);

    void clearPositionUpdated();
    bool positionIsUpdated() const;

	void startStimulation();
    void stopStimulation();

    std::vector<StimCircle> getCircles();
    void addCircle(StimCircle c);
    void editCircle(int ind, float x, float y, float rad, bool on);
    void deleteCircle(int ind);
    void disableCircles();
    // Circle setter can be done using Cicle class public methods
    int getSelectedCircle() const;
    void setSelectedCircle(int ind);

    bool getSimulateTrajectory() const;
	void setSimulateTrajectory(bool sim);

    int getOutputChan() const;
	void setOutputChan(int chan);

	int getSelectedStimSource() const;
	void setSelectedStimSource(int source);

    float getStimFreq() const;
	void setStimFreq(float stimFreq);

    float getStimSD() const;
	void setStimSD(float stimSD);

    stim_mode getStimMode() const;
	void setStimMode(stim_mode mode);

    int getTTLDuration() const;
    void setTTLDuration(int dur);

    /** Returns the circle number if postion is whithin a circle */
	int isPositionWithinCircles(float x, float y);
	
	StringArray colors = { "red",
							 "green",
							 "blue",
							 "magenta",
							 "cyan",
							 "orange",
							 "pink",
							 "grey",
							 "violet",
							 "yellow" };

private:
	int64 m_startingRecTimeMillis;
	int64 m_startingAcqTimeMillis;

	CriticalSection lock;

	bool m_positionIsUpdated;
	bool m_isRecordingTimeLogged;
	bool m_isAcquisitionTimeLogged;
	bool m_isInitialized = false;
	bool messageReceived;

	// Stim ON/OFF
	bool m_isOn;

	// Time stim
    float m_timePassed;
    int64 m_previousTime;
    int64 m_currentTime;
    bool m_ttlTriggered;

    std::default_random_engine generator;



    // Time sim position
    float m_timePassed_sim;
    int64 m_previousTime_sim;
    int64 m_currentTime_sim;
    int m_count;
    bool m_forward;
    float m_rad;
	bool m_simulateTrajectory;

    std::vector<StimCircle> m_circles;
    int m_selectedCircle;

    // Stimulation params
    float m_stimFreq;
    float m_stimSD;
    stim_mode m_stimMode;
    int m_pulseDuration;

    int m_outputChan; // Selected stimulation chan
	int m_selectedStimSource; // Selected stimulation source

	StreamSettings<TrackingNodeSettings> settings;

	OwnedArray<TrackingModule> trackers;

	void triggerEvent();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackingNode);
};

#endif