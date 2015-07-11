#include "EchoHandler.h"

namespace ASI
{

  EchoHandler::MidiEvent::MidiEvent(const jack_midi_data_t * data, const jack_nframes_t time)
    : m_time(time)
  {
    m_data[0] = (data[0] & 0xf0) + 3;   // cmd

    m_data[1] = data[1] - 20;   // note
    m_data[2] = data[2] + 20;   // velocity
  }

  EchoHandler::EchoHandler(jack_client_t * client, double lagSeconds)
    : m_client(client), m_lagSeconds(lagSeconds), m_lagFrames(0)
  {
    m_inputPort = jack_port_register(m_client, "echo_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    m_outputPort = jack_port_register (m_client, "echo_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
  }

  int EchoHandler::process(jack_nframes_t nframes)
  {
    void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
    void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

    jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

    jack_nframes_t framesAtStart = jack_last_frame_time(m_client);

    jack_midi_clear_buffer(outPortBuf);

    for(size_t i = 0; i < eventCount; ++i)
    {
      jack_midi_event_t inEvent;
      jack_midi_event_get(&inEvent, inPortBuf, i);

      jack_midi_data_t cmd = *inEvent.buffer & 0xf0;

      switch (cmd)
      {
      case 0x90: // note ON
      case 0x80: // note OFF
	{
	  const jack_nframes_t newTime = framesAtStart + inEvent.time + m_lagFrames;

	  m_queue.push_back(MidiEvent(inEvent.buffer, newTime));

	  break;
	}
      }
    }

    const jack_nframes_t lastFrame = framesAtStart + nframes;

    while (!m_queue.empty() && m_queue.front().m_time >= framesAtStart && m_queue.front().m_time < lastFrame)
    {
      const MidiEvent & event = m_queue.front();
      const jack_nframes_t newOffset = event.m_time - framesAtStart;
      jack_midi_event_write(outPortBuf, newOffset, event.m_data, 3);
      m_queue.pop_front();
    }

    return 0;
  }

  int EchoHandler::sampleRate(jack_nframes_t nframes)
  {
    m_lagFrames = m_lagSeconds * nframes;
    return 0;
  }

  void EchoHandler::shutdown()
  {
  }

}
