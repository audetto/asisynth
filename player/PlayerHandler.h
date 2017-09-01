#pragma once

#include "InputOutputHandler.h"
#include "MidiEvent.h"

#include <jack/midiport.h>
#include <vector>
#include <string>

namespace ASI
{

  class PortMapper;

  namespace Player
  {

    /*
      This is like a midi player, with melody from a json file
    */
    class PlayerHandler : public InputOutputHandler
    {
    public:

      PlayerHandler(jack_client_t * client, PortMapper & mapper, const std::string & filename, const size_t firstBeat);

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

    private:

      const size_t m_firstBeat;

      jack_transport_state_t m_previousState;

      std::vector<MidiEvent> m_master;
      size_t m_position;
    };

  }

}
