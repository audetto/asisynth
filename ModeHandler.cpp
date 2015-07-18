#include "ModeHandler.h"
#include "MidiCommands.h"
#include "MidiPassThrough.h"

#include <cstdlib>
#include <cassert>

namespace
{

  // all 7 notes of the major scale, can be converted to minor
  // of the other 5 alterations, 2 cannot
  // (e.g. between 3rd and 4th major note there is nothing,
  // while between 3rd and 4th minor there is one)
  //
  //                             0   1   2   3   4   5   6   7   8   9  10  11
  const int majorToMinor[12] = { 0,  1,  2, -1,  3,  5,  6,  7, -1,  8,  9, 10};
  const int minorToMajor[12] = { 0,  1,  2,  4, -1,  5,  6,  7,  9, 10, 11, -1};

  jack_midi_data_t transpose(const int offset, const int note, bool & ok)
  {
    const int noteAdj = note + offset;
    const std::div_t res = std::div(noteAdj, 12);

    const int x = majorToMinor[res.rem];

    if (x == -1)
    {
      // this accidental note (not in the canonical scale)
      // cannot be converted and will not be played
      ok = false;
      return 0;
    }
    else
    {
      const int newNote = -offset + res.quot * 12 + x;

      // check we are in the MIDI range, otherwise, do not play
      ok = newNote >= 0 && newNote < 128;
      return newNote;
    }
  }
}

namespace ASI
{

  ModeHandler::ModeHandler(jack_client_t * client, const int offset, const std::string & target)
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
  }

  int ModeHandler::process(const jack_nframes_t nframes)
  {
    void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
    void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

    jack_midi_clear_buffer(outPortBuf);

    if (!m_active)
    {
      return midiPassThrough(inPortBuf, outPortBuf, nframes, m_active);
    }

    jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

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
	  const jack_midi_data_t note = inEvent.buffer[1];

	  bool ok;
	  const jack_midi_data_t newNote = transpose(m_offset, note, ok);

	  if (ok)
	  {
	    jack_midi_data_t data[3];
	    data[0] = inEvent.buffer[0];
	    data[1] = newNote;
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
    return 0;
  }

  int ModeHandler::sampleRate(const jack_nframes_t nframes)
  {
    return 0;
  }

  void ModeHandler::shutdown()
  {
  }

}
