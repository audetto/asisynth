cmake_minimum_required(VERSION 3.3)

project(asisynth)

add_executable(asisynth
  AsiSynth.cpp
  DisplayHandler.cpp
  Factory.cpp
  I_JackHandler.cpp
  InputOutputHandler
  MidiEvent.cpp
  MidiPassThrough.cpp
  MidiUtils.cpp
  chords/ChordPlayerHandler.cpp
  echo/EchoHandler.cpp
  legato/SuperLegatoHandler.cpp
  mode/ModeHandler.cpp
  synth/IIRFactory.cpp
  synth/SynthesiserHandler.cpp
  synth/SynthParameters.cpp
  player/PlayerParameters.cpp
  player/PlayerHandler.cpp
  transport/TransportHandler.cpp
  )

add_library(sigproc
  sigproc/liir.c)

target_link_libraries(asisynth jack)
target_link_libraries(asisynth boost_program_options)
target_link_libraries(asisynth sigproc)

set_property(TARGET asisynth PROPERTY CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -march=native -ffast-math -funroll-loops -fassociative-math")

MESSAGE("Build type is:                   ${CMAKE_BUILD_TYPE}")
MESSAGE("CMAKE_CXX_FLAGS:                 ${CMAKE_CXX_FLAGS}")
MESSAGE("CMAKE_CXX_FLAGS_RELEASE:         ${CMAKE_CXX_FLAGS_RELEASE}")
MESSAGE("CMAKE_CXX_FLAGS_DEBUG:           ${CMAKE_CXX_FLAGS_DEBUG}")
MESSAGE("CMAKE_CXX_FLAGS_RELWIHDEBINFO:   ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
