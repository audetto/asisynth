#pragma once

#include "I_JackHandler.h"
#include "MidiEvent.h"

#include <jack/midiport.h>
#include <set>

namespace ASI
{

  /*
    This class delays NOTEOFF to achieve extra legato effect
   */
  class SuperLegatoHandler : public I_JackHandler
  {
  public:

    SuperLegatoHandler(jack_client_t * client, const int delayMilliseconds);

    virtual int process(const jack_nframes_t nframes);

    virtual int sampleRate(const jack_nframes_t nframes);

    virtual void shutdown();

  private:

    jack_client_t *m_client;
    const int m_delayMilliseconds;

    jack_nframes_t m_delayFrames;
    jack_port_t *m_inputPort;
    jack_port_t *m_outputPort;

    // these are sorted by event time
    std::multiset<MidiEvent> m_queue;

  };

}
