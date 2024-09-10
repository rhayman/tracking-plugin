cmake_minimum_required(VERSION 3.5.0)

include(../PluginRules.cmake)

add_sources(${PLUGIN_NAME}
	TrackingMessage.h
	TrackingNode.h
	TrackingNode.cpp
	TrackingNodeEditor.h
	TrackingNodeEditor.cpp
	TrackingStimulatorCanvas.h
	TrackingStimulatorCanvas.cpp
)

