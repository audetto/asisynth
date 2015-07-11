#pragma once

#include <jack/jack.h>

namespace ASI
{

  class I_JackHandler
  {
  public:
    virtual ~I_JackHandler();

    virtual int process(jack_nframes_t nframes) = 0;

    virtual int sampleRate(jack_nframes_t nframes) = 0;

    virtual void shutdown() = 0;
  };

}