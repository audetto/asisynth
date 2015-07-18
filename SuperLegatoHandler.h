#pragma once

#include "InputOutputHandler.h"
#include "MidiEvent.h"

#include <jack/midiport.h>
#include <set>

namespace ASI
{

  /*
    This class delays NOTEOFF to achieve extra legato effect
   */
  class SuperLegatoHandler : public InputOutputHandler
  {
  public:

    SuperLegatoHandler(jack_client_t * client, const int delayMilliseconds);

    virtual void process(const jack_nframes_t nframes);

    virtual void sampleRate(const jack_nframes_t nframes);

    virtual void shutdown();

  private:

    const int m_delayMilliseconds;

    jack_nframes_t m_delayFrames;

    // these are sorted by event time
    std::multiset<MidiEvent> m_queue;

  };

}
