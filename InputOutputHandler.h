#pragma once

#include "I_JackHandler.h"

namespace ASI
{

  class InputOutputHandler : public I_JackHandler
  {
  public:
    InputOutputHandler(jack_client_t * client);

  protected:

    jack_client_t *m_client;
    jack_port_t *m_inputPort;
    jack_port_t *m_outputPort;

    bool m_active;
  };

}
