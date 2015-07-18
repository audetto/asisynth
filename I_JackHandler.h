#pragma once

#include <jack/jack.h>

namespace ASI
{

  class I_JackHandler
  {
  public:
    virtual ~I_JackHandler();

    virtual void process(const jack_nframes_t nframes) = 0;

    virtual void sampleRate(const jack_nframes_t nframes) = 0;

    virtual void shutdown() = 0;
  };

}
