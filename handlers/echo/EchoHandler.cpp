#include "handlers/echo/EchoHandler.h"
#include "MidiCommands.h"
#include "CommonControls.h"

namespace
{
  ASI::MidiEvent createNewMidiEvent(const jack_nframes_t time, const jack_midi_event_t & org, const int transposition, const double velocityRatio)
  {
    int note = org.buffer[1];

    note += transposition;

    while (note < 0)
    {
      note += 12; // 1 octave
    }
    while (note >= 128)
    {
      note -= 12; // 1 octave
    }

    const jack_midi_data_t cmd = org.buffer[0];
    jack_midi_data_t velocity = org.buffer[2] * velocityRatio;
    velocity = std::min(jack_midi_data_t(127), velocity);

    return ASI::MidiEvent(time, cmd, note, velocity);
  }
}

namespace ASI
{
  namespace Echo
  {

    EchoHandler::EchoHandler(const std::shared_ptr<CommonControls> & common, const double lagSeconds, const int transposition, const double velocityRatio)
      : InputOutputHandler(common), m_transposition(transposition), m_velocityRatio(velocityRatio)
    {
      m_inputPort = m_common->registerPort("echo_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
      m_outputPort = m_common->registerPort("echo_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
      m_lagFrames = lagSeconds * m_sampleRate;
    }

    void EchoHandler::process(const jack_nframes_t nframes)
    {
      jack_client_t * client = m_common->getClient();

      void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
      void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

      jack_midi_clear_buffer(outPortBuf);

      const jack_transport_state_t state = jack_transport_query(client, nullptr);

      switch(state)
      {
      case JackTransportStopped:
	{
	  if (m_previousState != state)
	  {
	    // stop them all now
	    allNotesOff(outPortBuf, 0);
	    m_queue.clear();
	  }
	  break;
	}
      case JackTransportRolling:
	{
	  const jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

	  const jack_nframes_t framesAtStart = jack_last_frame_time(client);

	  for (size_t i = 0; i < eventCount; ++i)
	  {
	    jack_midi_event_t inEvent;
	    jack_midi_event_get(&inEvent, inPortBuf, i);

	    const jack_midi_data_t cmd = inEvent.buffer[0] & 0xf0;

	    switch (cmd)
	    {
	    case MIDI_NOTEON:
	    case MIDI_NOTEOFF:
	      {
		const jack_nframes_t newTime = framesAtStart + inEvent.time + m_lagFrames;

		m_queue.push_back(createNewMidiEvent(newTime, inEvent, m_transposition, m_velocityRatio));

		break;
	      }
	    }
	  }

	  const jack_nframes_t lastFrame = framesAtStart + nframes;

	  while (!m_queue.empty() && m_queue.front().m_time >= framesAtStart && m_queue.front().m_time < lastFrame)
	  {
	    const MidiEvent & event = m_queue.front();
	    const jack_nframes_t newOffset = event.m_time - framesAtStart;
	    jack_midi_event_write(outPortBuf, newOffset, event.m_data, event.m_size);
	    noteChange(event.m_data);
	    m_queue.pop_front();
	  }
	  break;
	}
      default:
	{
	  break;
	}

      }
    }

    void EchoHandler::shutdown()
    {
    }

  }
}
