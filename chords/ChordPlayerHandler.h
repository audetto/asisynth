#pragma once

#include "InputOutputHandler.h"
#include "MidiEvent.h"

#include <jack/midiport.h>
#include <string>
#include <vector>

namespace ASI
{

  class PortMapper;

  namespace Chords
  {

    /*
      Plays chords when the correct NOTEON is received
    */
    class ChordPlayerHandler : public InputOutputHandler
    {
    public:

      ChordPlayerHandler(jack_client_t * client, PortMapper & mapper, const std::string & filename, const int velocity);

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

      struct ChordData
      {
	jack_midi_data_t trigger;
	bool skip; // simply skip this data
	std::vector<jack_midi_data_t> notes;
      };

    private:

      const std::string m_filename;
      const int m_velocity;

      jack_transport_state_t m_previousState;

      std::vector<ChordData> m_chords;

      size_t m_next;
      size_t m_previous;

      void reset();
      void setNext(const size_t next);

    };

  }
}
