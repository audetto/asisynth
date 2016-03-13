#include "SuperLegatoHandler.h"
#include "../MidiCommands.h"

#include <cstdlib>
#include <memory>

namespace ASI
{
  namespace Legato
  {

    SuperLegatoHandler::SuperLegatoHandler(jack_client_t * client, const int delayMilliseconds)
      : InputOutputHandler(client)
    {
      m_inputPort = jack_port_register(m_client, "legato_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
      m_outputPort = jack_port_register (m_client, "legato_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

      if (delayMilliseconds < 0)
      {
	const std::string message = "Legato delay must be positive";
	throw std::runtime_error(message);
      }

      m_delayFrames = delayMilliseconds * m_sampleRate / 1000;
    }

    void SuperLegatoHandler::process(const jack_nframes_t nframes)
    {
      void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
      void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

      jack_midi_clear_buffer(outPortBuf);

      const jack_transport_state_t state = jack_transport_query(m_client, nullptr);

      const jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

      const jack_nframes_t framesAtStart = jack_last_frame_time(m_client);

      for (size_t i = 0; i < eventCount; ++i)
      {
	jack_midi_event_t inEvent;
	jack_midi_event_get(&inEvent, inPortBuf, i);

	const jack_midi_data_t cmd = inEvent.buffer[0] & 0xf0;
	const jack_midi_data_t velocity = inEvent.buffer[2];

	// NOTEOFF is delayed to achieve extra legato effect
	jack_nframes_t delay = 0;
	if (state == JackTransportRolling)
	{
	  if (cmd == MIDI_NOTEOFF || (cmd == MIDI_NOTEON && velocity == 0))
	  {
	    delay = m_delayFrames;
	  }
	}

	const jack_nframes_t newTime = framesAtStart + inEvent.time + delay;

	// they will be inserted in order (< of the multiset)
	m_queue.insert(MidiEvent(newTime, inEvent.buffer, inEvent.size));
      }

      const jack_nframes_t lastFrame = framesAtStart + nframes;

      // replay them in order
      while (!m_queue.empty() && m_queue.begin()->m_time >= framesAtStart && m_queue.begin()->m_time < lastFrame)
      {
	const std::multiset<MidiEvent>::iterator it = m_queue.begin();
	const MidiEvent & event = *it;
	const jack_nframes_t newOffset = event.m_time - framesAtStart;
	jack_midi_event_write(outPortBuf, newOffset, event.m_data, event.m_size);
	m_queue.erase(it);
      }
    }

    void SuperLegatoHandler::shutdown()
    {
    }

  }
}
