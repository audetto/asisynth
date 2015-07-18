#include "SuperLegatoHandler.h"
#include "MidiCommands.h"
#include "MidiPassThrough.h"

#include <cstdlib>
#include <cassert>
#include <memory>

namespace ASI
{

  SuperLegatoHandler::SuperLegatoHandler(jack_client_t * client, const int delayMilliseconds)
    : InputOutputHandler(client), m_delayMilliseconds(delayMilliseconds)
  {
    m_inputPort = jack_port_register(m_client, "legato_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    m_outputPort = jack_port_register (m_client, "legato_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    assert(m_delayMilliseconds >= 0);
  }

  void SuperLegatoHandler::process(const jack_nframes_t nframes)
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

      const jack_midi_data_t cmd = *inEvent.buffer & 0xf0;

      // NOTEOFF is delayed to achieve extra legato effect
      const jack_nframes_t delay = (cmd == MIDI_NOTEOFF) ? m_delayFrames : 0;

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

    if (!m_active)
    {
      m_queue.clear();
    }
  }

  void SuperLegatoHandler::sampleRate(const jack_nframes_t nframes)
  {
    m_delayFrames = m_delayMilliseconds * nframes / 1000;
  }

  void SuperLegatoHandler::shutdown()
  {
  }

}
