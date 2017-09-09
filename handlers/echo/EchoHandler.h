#pragma once

#include "handlers/InputOutputHandler.h"
#include "MidiEvent.h"

#include <jack/midiport.h>
#include <list>

namespace ASI
{
  namespace Echo
  {

    /*
      This class echoes all midi event coming in with a delay of lagSeconds
    */
    class EchoHandler : public InputOutputHandler
    {
    public:

      EchoHandler(const std::shared_ptr<CommonControls> & common, const double lagSeconds, const int transposition, const double velocityRatio);

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

    private:

      const int m_transposition;
      const double m_velocityRatio;

      jack_nframes_t m_lagFrames;

      jack_transport_state_t m_previousState;
      std::list<MidiEvent> m_queue;

    };

  }
}
