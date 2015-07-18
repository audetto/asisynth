#include "InputOutputHandler.h"

namespace ASI
{

  InputOutputHandler::InputOutputHandler(jack_client_t * client)
    : m_client(client), m_inputPort(nullptr), m_outputPort(nullptr), m_active(true)
  {
  }

}
