#include "CommonControls.h"

#include <jack/midiport.h>
#include <sstream>

namespace ASI
{

  CommonControls::CommonControls(jack_client_t * client, const bool simpleNames)
    : m_client(client), m_simpleNames(simpleNames), m_midiInputId(0), m_midiOutputId(0), m_audioInputId(0), m_audioOutputId(0)
  {
  }

  std::string CommonControls::getPortName(const char * port_name,
				      const char * port_type,
				      const unsigned long flags)
  {
    if (!m_simpleNames)
    {
      return port_name;
    }

    std::ostringstream buffer;

    if (port_type == JACK_DEFAULT_MIDI_TYPE)
    {
      if (flags == JackPortIsInput)
      {
	buffer << "midi input " << (++m_midiInputId);
      }
      else if (flags == JackPortIsOutput)
      {
	buffer << "midi output " << (++m_midiOutputId);
      }
    }
    else if (port_type == JACK_DEFAULT_AUDIO_TYPE)
    {
      if (flags == JackPortIsInput)
      {
	buffer << "audio input " << (++m_audioInputId);
      }
      else if (flags == JackPortIsOutput)
      {
	buffer << "audio output " << (++m_audioOutputId);
      }
    }

    const std::string name = buffer.str();
    if (name.empty())
    {
      return port_name;
    }

    return name;
  }

  jack_port_t * CommonControls::registerPort(const char * port_name,
					 const char * port_type,
					 const unsigned long flags)
  {
    const std::string name = getPortName(port_name, port_type, flags);
    jack_port_t * port = jack_port_register(m_client, name.c_str(), port_type, flags, 0);
    return port;
  }

  jack_client_t * CommonControls::getClient() const
  {
    return m_client;
  }

}
