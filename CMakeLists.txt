cmake_minimum_required(VERSION 3.3)

project(asisynth)

add_executable(asisynth
  AsiSynth.cpp
  CommonControls.cpp
  Factory.cpp
  I_JackHandler.cpp
  MidiEvent.cpp
  MidiUtils.cpp
  handlers/InputOutputHandler.cpp
  handlers/chords/ChordPlayerHandler.cpp
  handlers/display/DisplayHandler.cpp
  handlers/echo/EchoHandler.cpp
  handlers/legato/SuperLegatoHandler.cpp
  handlers/mode/ModeHandler.cpp
  handlers/player/PlayerParameters.cpp
  handlers/player/PlayerHandler.cpp
  handlers/server/ServerHandler.cpp
  handlers/synth/IIRFactory.cpp
  handlers/synth/SynthesiserHandler.cpp
  handlers/synth/SynthParameters.cpp
  handlers/transport/TransportHandler.cpp
  sounds/Sounds.cpp
  )

add_library(sigproc
  sigproc/liir.c)

include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(asisynth jack)
target_link_libraries(asisynth boost_program_options)
target_link_libraries(asisynth pthread)
target_link_libraries(asisynth zmq)
target_link_libraries(asisynth sigproc)

set_property(TARGET asisynth PROPERTY CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -march=native -ffast-math -funroll-loops -fassociative-math")

MESSAGE("CMAKE_BUILD_TYPE:                ${CMAKE_BUILD_TYPE}")
MESSAGE("CMAKE_CXX_FLAGS:                 ${CMAKE_CXX_FLAGS}")
MESSAGE("CMAKE_CXX_FLAGS_RELEASE:         ${CMAKE_CXX_FLAGS_RELEASE}")
MESSAGE("CMAKE_CXX_FLAGS_DEBUG:           ${CMAKE_CXX_FLAGS_DEBUG}")
MESSAGE("CMAKE_CXX_FLAGS_RELWIHDEBINFO:   ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
