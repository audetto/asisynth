#include "TransportHandler.h"
#include "../MidiPassThrough.h"

#include <cstring>

namespace ASI
{

  namespace Transport
  {
    TransportHandler::TransportHandler(jack_client_t * client)
      : InputOutputHandler(client), m_active(true)
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

	bool newActive = m_active;
	filtered(inEvent, newActive);

	if (newActive != m_active)
	{
	  // it has changed
	  if (newActive)
	  {
	    // it has just been activated
	    jack_transport_start(m_client);
	  }
	  else
	  {
	    // stop everything, and restart at zero for now
	    jack_transport_stop(m_client);
	    jack_position_t pos;
	    memset(&pos, 0, sizeof(pos));
	    pos.frame = 0;
	    jack_transport_reposition(m_client, &pos);
	  }
	  m_active = newActive;
	}
      }
    }

    void TransportHandler::shutdown()
    {
    }

  }
}
