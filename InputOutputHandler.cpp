#include "InputOutputHandler.h"
#include "MidiCommands.h"

namespace ASI
{

  InputOutputHandler::InputOutputHandler(jack_client_t * client)
    : m_client(client), m_inputPort(nullptr), m_outputPort(nullptr), m_active(true), m_notes(127, 0)
  {
  }

  void InputOutputHandler::noteChange(const jack_midi_data_t * data)
  {
    const jack_midi_data_t cmd = data[0] & 0xf0;
    const jack_midi_data_t n = data[1];

    switch (cmd)
    {
    case MIDI_NOTEON:
      {
	++m_notes[n];
	break;
      }
    case MIDI_NOTEOFF:
      {
	if (m_notes[n] > 0)
	{
	  --m_notes[n];
	}
	break;
      }
    }
  }

  void InputOutputHandler::allNotesOff(void * buffer, const jack_midi_data_t time)
  {
    jack_midi_data_t data[3];
    data[0] = MIDI_NOTEOFF;
    data[2] = 64;

    for (size_t i = 0; i < m_notes.size(); ++i)
    {
      if (m_notes[i] > 0)
      {
	data[1] = i;
	jack_midi_event_write(buffer, time, data, 3);
	m_notes[i] = 0;
      }
    }

  }

}
