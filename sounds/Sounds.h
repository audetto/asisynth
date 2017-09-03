#pragma once

#include <jack/midiport.h>

#include <string>
#include <map>

namespace ASI
{

  namespace Sounds
  {

    struct Program
    {
      jack_midi_data_t number; // 1 based
      jack_midi_data_t msb;
      jack_midi_data_t lsb;
    };

    const std::map<jack_midi_data_t, Program> & getPianoSounds(const std::string & name);

  }

}
