#pragma once

#include "I_JackHandler.h"
#include "MidiEvent.h"

#include <jack/midiport.h>
#include <list>

namespace ASI
{

  /*
    This class echoes all midi event coming in with a delay of lagSeconds
   */
  class EchoHandler : public I_JackHandler
  {
  public:

    EchoHandler(jack_client_t * client, const double lagSeconds, const int transposition);

    virtual int process(const jack_nframes_t nframes);

    virtual int sampleRate(const jack_nframes_t nframes);

    virtual void shutdown();

  private:

    jack_client_t *m_client;
    const double m_lagSeconds;
    const int m_transposition;

    jack_nframes_t m_lagFrames;
    jack_port_t *m_inputPort;
    jack_port_t *m_outputPort;

    std::list<MidiEvent> m_queue;

  };

}
