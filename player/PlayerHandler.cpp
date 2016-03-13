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

    PlayerHandler::PlayerHandler(jack_client_t * client, const std::string & filename, const size_t firstBeat)
      : InputOutputHandler(client), m_firstBeat(firstBeat)
    {
      m_outputPort = jack_port_register (m_client, "player_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

      const std::shared_ptr<const Melody> melody = loadPlayerMelody(filename);

      processMelody(*melody, m_sampleRate, m_firstBeat, m_master);
      m_position = 0;
      m_previousState = JackTransportStopped;
    }

    void PlayerHandler::process(const jack_nframes_t nframes)
    {
      void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

      jack_midi_clear_buffer(outPortBuf);

      jack_position_t pos;
      const jack_transport_state_t state = jack_transport_query(m_client, &pos);

      switch (state)
      {
      case JackTransportStopped:
	{
	  if (m_previousState != state)
	  {
	    // stop them all now
	    allNotesOff(outPortBuf, 0);
	    // reset position
	    m_position = 0;
	  }
	  break;
	}
      case JackTransportRolling:
	{
	  const jack_nframes_t framesAtStart = jack_last_frame_time(m_client);
	  const jack_nframes_t lastFrame = framesAtStart + nframes;

	  const jack_nframes_t startFrame = framesAtStart - pos.frame;

	  while (m_position < m_master.size() && startFrame + m_master[m_position].m_time >= framesAtStart && startFrame + m_master[m_position].m_time < lastFrame)
	  {
	    const MidiEvent & event = m_master[m_position];
	    const jack_nframes_t newOffset = event.m_time - pos.frame;
	    jack_midi_event_write(outPortBuf, newOffset, event.m_data, event.m_size);
	    noteChange(event.m_data);

	    ++m_position;
	  }
	  break;
	}
      default:
	{
	  break;
	}
      }

      m_previousState = state;
    }

    void PlayerHandler::shutdown()
    {
    }

  }
}
