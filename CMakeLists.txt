project(asisynth)

add_executable(asisynth
  AsiSynth.cpp
  I_JackHandler.cpp
  EchoHandler.cpp
  Factory.cpp
  MidiEvent.cpp
  ModeHandler.cpp)

target_link_libraries(asisynth jack)
target_link_libraries(asisynth boost_program_options)

set_property(TARGET asisynth PROPERTY CXX_STANDARD 11)
