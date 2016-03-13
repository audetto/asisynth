#include "ModeHandler.h"
#include "../MidiCommands.h"

#include <cstdlib>
#include <cassert>

namespace
{

  // all 7 notes of the major scale, can be converted to minor
  // of the other 5 alterations, 2 cannot
  // (e.g. between 3rd and 4th major note there is nothing,
  // while between 3rd and 4th minor there is one)
  //
  //                             0   1   2   3   4   5   6   7   8   9  10  11, 12
  const int majorToMinor[13] = { 0,  1,  2, -1,  3,  5,  6,  7, -1,  8,  9, 10, 12};
  const int minorToMajor[13] = { 0,  1,  2,  4, -1,  5,  6,  7,  9, 10, 11, -1, 12};

  const int NOT_MAPPED = -1;
  const int SKIP = -2;

  // returns -2 to indicate silence
  int transpose(const int offset, const int quirkOffset, const int note)
  {
    const int noteAdj = note + offset;
    const std::div_t res = std::div(noteAdj, 12);

    int x = majorToMinor[res.rem];

    if (x == -1)
    {
      // try again with quirk
      x = majorToMinor[res.rem + quirkOffset];
      if (x == -1)
      {
	// this accidental note (not in the canonical scale)
	// cannot be converted and will not be played
	return SKIP;
      }
    }

    const int newNote = -offset + res.quot * 12 + x;

    // check we are in the MIDI range, otherwise, do not play
    if (newNote >= 0 && newNote < 128)
      return newNote;
    else
      return SKIP;
  }
}

namespace ASI
{
  namespace Mode
  {

    ModeHandler::ModeHandler(jack_client_t * client, const int offset, const std::string & target, const std::string & quirk)
      : InputOutputHandler(client), m_offset(offset % 12)
    {
      m_inputPort = jack_port_register(m_client, "mode_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
      m_outputPort = jack_port_register (m_client, "mode_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

      if (target == "minor")
      {
	m_conversion = majorToMinor;
      }
      else if (target == "major")
      {
	m_conversion = minorToMajor;
      }
      else
      {
	assert(false);
      }

      if (quirk == "below")
      {
	m_quirkOffset = -1;
      }
      else if (quirk == "skip")
      {
	m_quirkOffset = 0;
      }
      else if (quirk == "above")
      {
	m_quirkOffset = 1;
      }
      else
      {
	assert(false);
      }

      m_mappedNotes.resize(128, -1);
    }

    void ModeHandler::process(const jack_nframes_t nframes)
    {
      void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
      void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

      jack_midi_clear_buffer(outPortBuf);

      const jack_transport_state_t state = jack_transport_query(m_client, nullptr);

      const jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

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
	    const jack_midi_data_t note = inEvent.buffer[1];
	    const jack_midi_data_t velocity = inEvent.buffer[2];

	    int noteToUse = note;

	    // we only map on a note on
	    // on a note off we use the previously mapped note
	    // and only is transport is rolling
	    if (cmd == MIDI_NOTEON && velocity > 0)
	    {
	      if (state == JackTransportRolling)
	      {
		const int newNote = transpose(m_offset, m_quirkOffset, note);

		m_mappedNotes[note] = newNote;
		noteToUse = newNote;
	      }
	    }
	    else
	    {
	      // NOTEOFF
	      if (m_mappedNotes[note] != NOT_MAPPED)
	      {
		noteToUse = m_mappedNotes[note];
		m_mappedNotes[note] = NOT_MAPPED;
	      }
	    }

	    if (noteToUse != SKIP)
	    {
	      jack_midi_data_t data[3];
	      data[0] = inEvent.buffer[0];
	      data[1] = noteToUse;;
	      data[2] = inEvent.buffer[2];

	      jack_midi_event_write(outPortBuf, inEvent.time, data, 3);
	    }
	    break;
	  }
	default:
	  {
	    // just forward everything else
	    jack_midi_event_write(outPortBuf, inEvent.time, inEvent.buffer, inEvent.size);
	  }
	}
      }
    }

    void ModeHandler::shutdown()
    {
    }

  }
}
