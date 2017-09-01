#pragma once

#include <jack/jack.h>
#include <string>

namespace ASI
{

  class PortMapper
  {
  public:
    PortMapper(jack_client_t * client, const bool simpleNames);

    jack_port_t * registerPort(const char * port_name,
			       const char * port_type,
			       const unsigned long flags);

  private:
    std::string getPortName(const char * port_name,
			    const char * port_type,
			    const unsigned long flags);

    jack_client_t * m_client;
    const bool m_simpleNames;

    size_t m_midiInputId;
    size_t m_midiOutputId;
    size_t m_audioInputId;
    size_t m_audioOutputId;
  };

}
