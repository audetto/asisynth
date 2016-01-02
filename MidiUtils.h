#pragma once

#include <string>
#include <ostream>

#include <jack/midiport.h>

namespace ASI
{
  enum NoteNamePreference
    {
      SHARP,
      FLAT,
      BEST
    };

  void streamNoteName(std::ostream & s, const jack_midi_data_t note, const NoteNamePreference preference);
}
