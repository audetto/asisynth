#include "InputOutputHandler.h"

namespace ASI
{

  InputOutputHandler::InputOutputHandler(jack_client_t * client)
    : m_client(client), m_inputPort(NULL), m_outputPort(NULL), m_active(true)
  {
  }

}
