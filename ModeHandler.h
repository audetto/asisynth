#pragma once

#include "InputOutputHandler.h"
#include "MidiEvent.h"

#include <string>

namespace ASI
{

  /*
    This class transposes Major <-> Minor
   */
  class ModeHandler : public InputOutputHandler
  {
  public:

    ModeHandler(jack_client_t * client, const int offset, const std::string & target);

    virtual void process(const jack_nframes_t nframes);

    virtual void sampleRate(const jack_nframes_t nframes);

    virtual void shutdown();

  private:

    const int m_offset; // 0 C, 1 B, 2 B flat....

    const int * m_conversion;

  };

}
