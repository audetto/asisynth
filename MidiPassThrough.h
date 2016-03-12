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

}
