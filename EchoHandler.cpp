#include "EchoHandler.h"
#include "MidiCommands.h"
#include "MidiPassThrough.h"

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

  EchoHandler::EchoHandler(jack_client_t * client, const double lagSeconds, const int transposition, const double velocityRatio)
    : InputOutputHandler(client), m_lagSeconds(lagSeconds), m_transposition(transposition), m_velocityRatio(velocityRatio), m_lagFrames(0)
  {
    m_inputPort = jack_port_register(m_client, "echo_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    m_outputPort = jack_port_register (m_client, "echo_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
  }

  void EchoHandler::process(const jack_nframes_t nframes)
  {
    void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
    void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

    jack_midi_clear_buffer(outPortBuf);

    if (!m_active)
    {
      return midiPassThrough(inPortBuf, outPortBuf, nframes, m_active);
    }

    jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

    jack_nframes_t framesAtStart = jack_last_frame_time(m_client);

    for(size_t i = 0; i < eventCount; ++i)
    {
      jack_midi_event_t inEvent;
      jack_midi_event_get(&inEvent, inPortBuf, i);

      if (filtered(inEvent, m_active))
      {
	continue;
      }

      jack_midi_data_t cmd = *inEvent.buffer & 0xf0;

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
      m_queue.pop_front();
    }

    if (!m_active)
    {
      m_queue.clear();
    }
  }

  void EchoHandler::sampleRate(const jack_nframes_t nframes)
  {
    m_lagFrames = m_lagSeconds * nframes;
  }

  void EchoHandler::shutdown()
  {
  }

}
