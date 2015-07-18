#include "InputOutputHandler.h"

namespace ASI
{
  InputOutputHandler::InputOutputHandler(jack_client_t * client)
    : m_client(client), m_inputPort(NULL), m_outputPort(NULL)
  {
  }

  InputOutputHandler::~InputOutputHandler()
  {
    jack_port_unregister(m_client, m_inputPort);
    jack_port_unregister(m_client, m_outputPort);
  }

}
