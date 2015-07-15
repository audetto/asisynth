#include "MidiEvent.h"

#include <cassert>

namespace ASI
{

  MidiEvent::MidiEvent(const jack_nframes_t time, const jack_midi_data_t * data, const size_t size)
    : m_time(time), m_size(size)
  {
    switch (size)
    {
    case 4: m_data[3] = data[3];   // extra
    case 3: m_data[2] = data[2];   // velocity
    case 2: m_data[1] = data[1];   // note
    case 1: m_data[0] = data[0];   // cmd
      break;
    default:
      assert(size <= 4);
    }
  }

  MidiEvent::MidiEvent(const jack_nframes_t time, const jack_midi_data_t cmd, const jack_midi_data_t note, const jack_midi_data_t velocity)
    : m_time(time), m_size(3)
  {
    m_data[0] = cmd;
    m_data[1] = note;
    m_data[2] = velocity;
  }

}
