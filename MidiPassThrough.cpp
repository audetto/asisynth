#include "MidiPassThrough.h"
#include "MidiCommands.h"
#include <iostream>

namespace ASI
{

  bool filtered(const jack_midi_event_t & event, bool & active)
  {
    const jack_midi_data_t cmd = *event.buffer & 0xf0;

    if (cmd == MIDI_CC)
    {
      const jack_midi_data_t control = event.buffer[1];
      if (control == 66)
      {
	const jack_midi_data_t value = event.buffer[2];

	active = value < 64;
	std::cout << "Filtered " << active << std::endl;
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

  void midiPassThrough(void* inPortBuf, void* outPortBuf, const jack_nframes_t nframes, bool & active)
  {
    std::cout << "Inactive" << std::endl;
    const jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

    for(size_t i = 0; i < eventCount; ++i)
    {
      jack_midi_event_t inEvent;
      jack_midi_event_get(&inEvent, inPortBuf, i);

      const bool flt = filtered(inEvent, active);

      if (!flt)
      {
	// just forward everything else
	jack_midi_event_write(outPortBuf, inEvent.time, inEvent.buffer, inEvent.size);
      }
    }
  }

}
