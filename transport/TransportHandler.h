#pragma once

#include "../InputOutputHandler.h"

namespace ASI
{
  namespace Transport
  {

    /*
      This class controls Jackd transport
    */
    class TransportHandler : public InputOutputHandler
    {
    public:

      TransportHandler(jack_client_t * client);

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

    private:

      bool m_pedalDown;
    };

  }
}
