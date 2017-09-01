#pragma once

#include "I_JackHandler.h"

#include <jack/midiport.h>
#include <string>
#include <vector>
#include <memory>

namespace ASI
{

  class PortMapper;

  namespace Display
  {

    class DisplayHandler : public I_JackHandler
    {
    public:

      DisplayHandler(jack_client_t * client, PortMapper & mapper, const std::string & filename);

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

    private:

      jack_client_t *m_client;
      jack_port_t *m_inputPort;

      std::shared_ptr<std::ostream> m_output;

      // so the first not is at time = 0.0
      double m_offset;

      std::vector<double> m_onTimes;
    };

  }
}
