#pragma once

#include <jack/jack.h>
#include <jack/midiport.h>
#include <string>
#include <memory>
#include <map>

namespace ASI
{

  namespace Sounds
  {

    struct Program;

  }

  class CommonControls
  {
  public:
    // input channel is 1 based
    CommonControls(jack_client_t * client, const bool simpleNames, const jack_midi_data_t channel, const std::string & piano);

    jack_port_t * registerPort(const char * port_name,
			       const char * port_type,
			       const unsigned long flags);

    jack_client_t * getClient() const;

    // channel is 1 based
    jack_midi_data_t getChannel() const;

    const std::map<jack_midi_data_t, Sounds::Program> & getSelectedSounds() const;

  private:
    std::string getPortName(const char * port_name,
			    const char * port_type,
			    const unsigned long flags);

    jack_client_t * m_client;
    const bool m_simpleNames;
    const int m_channel;
    const std::map<jack_midi_data_t, Sounds::Program> & m_sounds;

    size_t m_midiInputId;
    size_t m_midiOutputId;
    size_t m_audioInputId;
    size_t m_audioOutputId;
  };

}
