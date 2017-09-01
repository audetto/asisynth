#include "transport/TransportHandler.h"
#include "MidiCommands.h"

#include <cstring>

namespace ASI
{

  namespace Transport
  {
    TransportHandler::TransportHandler(jack_client_t * client)
      : InputOutputHandler(client), m_pedalDown(false)
    {
      m_inputPort = jack_port_register(m_client, "transport_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    }

    void TransportHandler::process(const jack_nframes_t nframes)
    {
      void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);

      jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

      for (size_t i = 0; i < eventCount; ++i)
      {
	jack_midi_event_t inEvent;
	jack_midi_event_get(&inEvent, inPortBuf, i);

	const jack_midi_data_t cmd = inEvent.buffer[0] & 0xf0;

	if (cmd == MIDI_CC)
	{
	  const jack_midi_data_t control = inEvent.buffer[1];
	  if (control == MIDI_CC_SOSTENUTO)
	  {
	    const jack_midi_data_t value = inEvent.buffer[2];

	    const bool newPedalDown = value >= 64;
	    if (newPedalDown != m_pedalDown)
	    {
	      // it has changed
	      if (newPedalDown)
	      {
		// stop everything, and restart at zero for now
		jack_transport_stop(m_client);
		jack_position_t pos;
		memset(&pos, 0, sizeof(pos));
		pos.frame = 0;
		jack_transport_reposition(m_client, &pos);
	      }
	      else
	      {
		// it has just been activated
		jack_transport_start(m_client);
	      }
	      m_pedalDown = newPedalDown;
	    }
	  }
	}
      }

    }

    void TransportHandler::shutdown()
    {
    }

  }
}
