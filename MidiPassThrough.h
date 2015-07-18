#pragma once

#include <jack/midiport.h>

namespace ASI
{

  // the middle pedal is used to switch off events, and turn the handler
  // in a passthrough
  // returns if the event was of these middle pedal
  // and updates the active flag
  // when the pedal is pressed, active = false
  bool filtered(const jack_midi_event_t & event, bool & active);

  // execute the passthrough and update the active flag (i.e. it switches it off in case);
  void midiPassThrough(void* inPortBuf, void* outPortBuf, const jack_nframes_t nframes, bool & active);

}
