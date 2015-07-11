#include "MidiEvent.h"

namespace ASI
{
  MidiEvent::MidiEvent(const jack_nframes_t time, const jack_midi_data_t * data)
    : m_time(time)
  {
    m_data[0] = data[0];   // cmd
    m_data[1] = data[1];   // note
    m_data[2] = data[2];   // velocity
  }

  MidiEvent::MidiEvent(const jack_nframes_t time, const jack_midi_data_t cmd, const jack_midi_data_t note, const jack_midi_data_t velocity)
    : m_time(time)
  {
    m_data[0] = cmd;
    m_data[1] = note;
    m_data[2] = velocity;
  }

}
