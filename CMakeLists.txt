project(asisynth)

add_executable(asisynth AsiSynth.cpp I_JackHandler.cpp EchoHandler.cpp)

target_link_libraries(asisynth jack)

set_property(TARGET asisynth PROPERTY CXX_STANDARD 11)
