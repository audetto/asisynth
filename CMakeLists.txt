cmake_minimum_required(VERSION 3.3)

project(asisynth)

add_executable(asisynth
  AsiSynth.cpp
  ChordPlayerHandler.cpp
  DisplayHandler.cpp
  EchoHandler.cpp
  Factory.cpp
  I_JackHandler.cpp
  IIRFactory.cpp
  InputOutputHandler
  MidiEvent.cpp
  MidiPassThrough.cpp
  MidiUtils.cpp
  ModeHandler.cpp
  SuperLegatoHandler.cpp
  SynthesiserHandler.cpp
  SynthParameters.cpp
  )

add_library(sigproc
  sigproc/liir.c)

target_link_libraries(asisynth jack)
target_link_libraries(asisynth boost_program_options)
target_link_libraries(asisynth sigproc)

set_property(TARGET asisynth PROPERTY CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -march=native -ffast-math")
