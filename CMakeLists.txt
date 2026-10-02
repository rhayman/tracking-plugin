cmake_minimum_required(VERSION 3.15)

include(../PluginRules.cmake)

add_sources(${PLUGIN_NAME}
  TrackingMessage.h
  TrackingNode.cpp
  TrackingNode.h
  TrackingNodeEditor.cpp
  TrackingNodeEditor.h
  TrackingStimulatorCanvas.cpp
  TrackingStimulatorCanvas.h
)
