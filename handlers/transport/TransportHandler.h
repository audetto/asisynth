#pragma once

#include "handlers/InputOutputHandler.h"

namespace ASI
{

  class CommonControls;

  namespace Transport
  {

    /*
      This class controls Jackd transport
    */
    class TransportHandler : public InputOutputHandler
    {
    public:

      TransportHandler(const std::shared_ptr<CommonControls> & common);

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

    private:

      bool m_pedalDown;
    };

  }
}
