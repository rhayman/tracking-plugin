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

#include "TrackingMessage.h"
#include <ProcessorHeaders.h>

#include <queue>
#include <random>
#include <stdio.h>
#include <utility>

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

    void push (const TrackingData& message);
    TrackingData* pop();

    bool isEmpty();
    void clear();

    int count();

private:
    TrackingData m_buffer[BUFFER_SIZE];
    int m_head;
    int m_tail;
    int _count = 0;
};

//	This helper class is an OSC Receiver running its own thread with RealtimeCallback.

class TrackingNode;

class TrackingModule : private OSCReceiver, private OSCReceiver::ListenerWithOSCAddress<OSCReceiver::RealtimeCallback>
{
public:
    TrackingModule (String name, int port, String address, String color, TrackingNode* processor);

    ~TrackingModule() {}

    void oscMessageReceived (const OSCMessage& message) override;

    friend std::ostream& operator<< (std::ostream&, const TrackingModule&);

    String m_name;
    int m_port = DEF_PORT;
    String m_address = String (DEF_ADDRESS);
    String m_color = String (DEF_COLOR);

    bool isConnected = false;

    std::unique_ptr<TrackingQueue> m_messageQueue;

    std::vector<TrackingPosition> positionData;

    TrackingNode* m_processor;

    TrackingSources source;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackingModule);
};

/**

  Class for Abstrac Stimulation Area

*/
class StimArea
{
public:
    StimArea();
    StimArea (float x, float y, bool on);

    float getX();
    float getY();
    bool getOn();
    void setX (float x);
    void setY (float y);

    bool on();
    bool off();

    virtual bool isPositionIn (float x, float y) = 0;
    virtual float distanceFromCenter (float x, float y) = 0;
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
    StimCircle (float x, float y, float r, bool on);

    float getRad();

    void setRad (float rad);
    void set (float x, float y, float rad, bool on);

    bool isPositionIn (float x, float y);
    float distanceFromCenter (float x, float y);
    String returnType();

private:
    float m_rad;
};

class TrackingNode : public GenericProcessor
{
public:
    /** The class constructor, used to initialize any members. */
    TrackingNode();

    /** The class destructor, used to deallocate memory */
    ~TrackingNode() {}

    // ------------------------------------------------------------
    //                   VIRTUAL METHODS
    // ------------------------------------------------------------

    /** Create the processor's custom editor. */
    AudioProcessorEditor* createEditor() override;

    /** Registers the parameters of the processor. */
    void registerParameters() override;

    /** Called when a parameter value is updated. */
    void parameterValueChanged (Parameter* param) override;

    /** Called when upstream signal chain changes. */
    void updateSettings() override;

    /** Called when acquisition starts. */
    bool startAcquisition() override;

    /** Called when acquisition stops. */
    bool stopAcquisition() override;

    /** Core processing callback — drains the OSC queue and fires TTL events. */
    void process (AudioBuffer<float>& buffer) override;

    // Creates a tracking module and adds a tracking source
    bool addSource (String name, int port = 0, String address = "", String color = "");

    void removeSource (int index);

    // receives a message from the osc server
    void receiveMessage (int port, String address, const TrackingData& message);

    // Setter-Getters

    int getPort (int idx);
    void setPort (int idx, int port);

    String getAddress (int idx);
    void setAddress (int i, String address);

    void setColor (int i, String color);
    String getColor (int i);

    int getNumSources();

    TrackingSources& getTrackingSource (int i);

    // Get the latest tracking positions
    std::vector<TrackingPosition> getTrackingPositions (int sourceIndex);

    void clearPositionUpdated();
    bool positionIsUpdated() const;

    void startStimulation();
    void stopStimulation();

    std::vector<StimCircle> getCircles();
    void addCircle (StimCircle c);
    void editCircle (int ind, float x, float y, float rad, bool on);
    void deleteCircle (int ind);
    void disableCircles();
    // Circle setter can be done using Cicle class public methods
    int getSelectedCircle() const;
    void setSelectedCircle (int ind);

    bool getSimulateTrajectory() const;
    void setSimulateTrajectory (bool sim);

    int getOutputChan() const;
    void setOutputChan (int chan);

    int getSelectedStimSource() const;
    void setSelectedStimSource (int source);

    float getStimFreq() const;
    void setStimFreq (float stimFreq);

    float getStimSD() const;
    void setStimSD (float stimSD);

    stim_mode getStimMode() const;
    void setStimMode (stim_mode mode);

    int getTTLDuration() const;
    void setTTLDuration (int dur);

    /** Returns the circle number if postion is whithin a circle */
    int isPositionWithinCircles (float x, float y);

    Array<String> colors = { "red",
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
    CriticalSection lock;

    bool m_positionIsUpdated;
    bool m_hasPendingMessages;

    // Stim ON/OFF
    bool m_isOn;

    // Time stim
    float m_timePassed;
    int64 m_previousTime;
    int64 m_currentTime;
    bool m_ttlTriggered;
    bool m_ttlIsOn;   // true while a TTL pulse is active
    int64 m_ttlOnTime; // system time (ms) when the TTL was turned on

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

    OwnedArray<TrackingModule> trackers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackingNode);
};

#endif