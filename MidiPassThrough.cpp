#include "MidiPassThrough.h"
#include "MidiCommands.h"

namespace ASI
{

  bool filtered(const jack_midi_event_t & event, bool & active)
  {
    const jack_midi_data_t cmd = event.buffer[0] & 0xf0;

    if (cmd == MIDI_CC)
    {
      const jack_midi_data_t control = event.buffer[1];
      if (control == MIDI_CC_SOSTENUTO)
      {
	const jack_midi_data_t value = event.buffer[2];

	active = value < 64;
	return true;
      }
      else
      {
	return false;
      }
    }
    else
    {
      return false;
    }
  }

}
