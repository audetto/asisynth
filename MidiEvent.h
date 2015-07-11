#pragma once

#include <jack/midiport.h>

namespace ASI
{

  struct MidiEvent
  {
    MidiEvent(const jack_nframes_t time, const jack_midi_data_t * data);
    MidiEvent(const jack_nframes_t time, const jack_midi_data_t cmd, const jack_midi_data_t note, const jack_midi_data_t velocity);

    jack_nframes_t m_time;
    jack_midi_data_t m_data[3];
  };

}
