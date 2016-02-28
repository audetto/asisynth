#include "PlayerHandler.h"
#include "PlayerParameters.h"
#include "../MidiCommands.h"
#include "../MidiPassThrough.h"

#include <algorithm>

namespace
{

  void processMelody(const ASI::Player::Melody & melody, const size_t sampleRate, std::vector<ASI::MidiEvent> & events)
  {
    events.clear();

    size_t beat = 0;
    for (const ASI::Player::Bar & bar : melody.bars)
    {
      const size_t repeat = bar.repeat;
      for (size_t i = 0; i < repeat; ++i)
      {
	for (const ASI::Player::Chord & chord : bar.chords)
	{
	  const size_t start = beat * 60 * sampleRate / melody.tempo;
	  const size_t end = (beat + chord.duration) * 60 * sampleRate / melody.tempo;
	  const size_t adjustedEnd = start + (end - start) * melody.legatoCoeff;
	  for (const size_t note : chord.notes)
	  {
	    events.emplace_back(start, MIDI_NOTEON, note, chord.velocity);
	    events.emplace_back(adjustedEnd, MIDI_NOTEOFF, note, chord.velocity);
	  }
	  ++beat;
	}
      }
    }

    std::sort(events.begin(), events.end());
  }

}

namespace ASI
{
  namespace Player
  {

    PlayerHandler::PlayerHandler(jack_client_t * client, const std::string & melodyFile)
      : InputOutputHandler(client)
    {
      m_inputPort = jack_port_register(m_client, "player_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
      m_outputPort = jack_port_register (m_client, "player_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

      m_melody = loadPlayerMelody(melodyFile);
    }

    void PlayerHandler::process(const jack_nframes_t nframes)
    {
      void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
      void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

      jack_midi_clear_buffer(outPortBuf);

      jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

      jack_nframes_t framesAtStart = jack_last_frame_time(m_client);

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
	    m_startFrame = framesAtStart + inEvent.time;
	    m_position = 0;
	  }
	  else
	  {
	    m_position = m_master.size();
	  }
	  m_active = newActive;
	}
      }

      const jack_nframes_t lastFrame = framesAtStart + nframes;

      while (m_position < m_master.size() && m_startFrame + m_master[m_position].m_time >= framesAtStart && m_startFrame + m_master[m_position].m_time < lastFrame)
      {
	const MidiEvent & event = m_master[m_position];
	const jack_nframes_t newOffset = m_startFrame + event.m_time - framesAtStart;
	jack_midi_event_write(outPortBuf, newOffset, event.m_data, event.m_size);
	++m_position;
      }
    }

    void PlayerHandler::sampleRate(const jack_nframes_t nframes)
    {
      processMelody(*m_melody, nframes, m_master);
      m_position = m_master.size();
    }

    void PlayerHandler::shutdown()
    {
    }

  }
}
