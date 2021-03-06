#pragma once

#include <jack/midiport.h>

namespace ASI
{

  struct MidiEvent
  {
    MidiEvent(const jack_nframes_t time, const jack_midi_data_t * data, const size_t size);
    MidiEvent(const jack_nframes_t time, const jack_midi_data_t cmd, const jack_midi_data_t note, const jack_midi_data_t velocity);

    jack_nframes_t m_time;
    size_t m_size;
    jack_midi_data_t m_data[4];

    bool operator< (const MidiEvent & rhs) const;
  };

}
