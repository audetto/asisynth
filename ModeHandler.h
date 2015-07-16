#pragma once

#include "I_JackHandler.h"
#include "MidiEvent.h"

#include <string>

namespace ASI
{

  /*
    This class transposes Major <-> Minor
   */
  class ModeHandler : public I_JackHandler
  {
  public:

    ModeHandler(jack_client_t * client, const int offset, const std::string & target);

    virtual int process(const jack_nframes_t nframes);

    virtual int sampleRate(const jack_nframes_t nframes);

    virtual void shutdown();

  private:

    jack_client_t *m_client;

    const int m_offset; // 0 C, 1 B, 2 B flat....

    jack_port_t *m_inputPort;
    jack_port_t *m_outputPort;

    const int * m_conversion;

  };

}
