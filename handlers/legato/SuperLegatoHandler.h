#pragma once

#include "handlers/InputOutputHandler.h"
#include "MidiEvent.h"

#include <jack/midiport.h>
#include <set>

namespace ASI
{

  namespace Legato
  {

    /*
      This class delays NOTEOFF to achieve extra legato effect
    */
    class SuperLegatoHandler : public InputOutputHandler
    {
    public:

      SuperLegatoHandler(const std::shared_ptr<CommonControls> & common, const int delayMilliseconds);

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

    private:

      jack_nframes_t m_delayFrames;

      // these are sorted by event time
      std::multiset<MidiEvent> m_queue;

    };

  }
}
