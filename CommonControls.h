#pragma once

#include <jack/jack.h>
#include <jack/midiport.h>
#include <string>
#include <memory>

namespace ASI
{

  class CommonControls
  {
  public:
    CommonControls(jack_client_t * client, const bool simpleNames, const jack_midi_data_t channel);

    jack_port_t * registerPort(const char * port_name,
			       const char * port_type,
			       const unsigned long flags);

    jack_client_t * getClient() const;

    jack_midi_data_t getChannel() const;

  private:
    std::string getPortName(const char * port_name,
			    const char * port_type,
			    const unsigned long flags);

    jack_client_t * m_client;
    const bool m_simpleNames;
    const int m_channel;

    size_t m_midiInputId;
    size_t m_midiOutputId;
    size_t m_audioInputId;
    size_t m_audioOutputId;
  };

}
