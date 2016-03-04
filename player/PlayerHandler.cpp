#include "PlayerHandler.h"
#include "PlayerParameters.h"
#include "../MidiCommands.h"
#include "../MidiPassThrough.h"

#include <algorithm>

namespace
{

  size_t getVelocity(const size_t beat, const std::vector<size_t> & velocity)
  {
    const size_t period = velocity.size();
    const size_t position = beat % period;
    return velocity[position];
  }

  void processMelody(const ASI::Player::Melody & melody, const size_t sampleRate, const size_t firstBeat, std::vector<ASI::MidiEvent> & events)
  {
    events.clear();

    size_t beat = 0;
    for (const ASI::Player::Chord & chord : melody.chords)
    {
      if (beat >= firstBeat)
      {
	const size_t adjBeat = beat - firstBeat;
	const size_t velocity = getVelocity(beat, melody.velocity);

	const size_t start = adjBeat * 60 * sampleRate / melody.tempo;
	const size_t end = (adjBeat + chord.duration) * 60 * sampleRate / melody.tempo;
	const size_t adjustedEnd = start + (end - start) * melody.legatoCoeff;
	for (const size_t note : chord.notes)
	{
	  events.emplace_back(start, MIDI_NOTEON, note, velocity);
	  events.emplace_back(adjustedEnd, MIDI_NOTEOFF, note, velocity);
	}
      }
      ++beat;
    }

    std::sort(events.begin(), events.end());
  }

}

namespace ASI
{
  namespace Player
  {

    PlayerHandler::PlayerHandler(jack_client_t * client, const std::string & melodyFile, const size_t firstBeat)
      : InputOutputHandler(client), m_firstBeat(firstBeat)
    {
      m_inputPort = jack_port_register(m_client, "player_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
      m_outputPort = jack_port_register (m_client, "player_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

      const std::shared_ptr<const Melody> melody = loadPlayerMelody(melodyFile);

      processMelody(*melody, m_sampleRate, m_firstBeat, m_master);
      // start in a "paused" position
      m_position = m_master.size();
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
	    // stop everything
	    allNotesOff(outPortBuf, inEvent.time);
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
	noteChange(event.m_data);

	++m_position;
      }
    }

    void PlayerHandler::shutdown()
    {
    }

  }
}
