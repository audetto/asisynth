#include "DisplayHandler.h"
#include "../MidiCommands.h"
#include "../MidiUtils.h"

#include <iostream>
#include <fstream>

namespace ASI
{

  namespace Display
  {

    DisplayHandler::DisplayHandler(jack_client_t * client, const std::string & filename)
      : m_client(client)
    {
      m_inputPort = jack_port_register(m_client, "display_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
      m_offset = 0.0;
      m_onTimes.resize(256, 0.0);

      if (filename == "-")
      {
	// no deleter
	m_output.reset(&std::cout, [](std::ostream*){});
      }
      else
      {
	m_output = std::make_shared<std::ofstream>(filename);
      }

      // write header
      *m_output << "Time,Code,Command,Note,Velocity,Name,On,Duration" << std::endl;
    }

    void DisplayHandler::process(const jack_nframes_t nframes)
    {
      void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);

      jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

      jack_nframes_t framesAtStart = jack_last_frame_time(m_client);

      if (eventCount > 0 && m_offset == 0.0)
      {
	m_offset = jack_frames_to_time(m_client, framesAtStart);
      }

      for(size_t i = 0; i < eventCount; ++i)
      {
	jack_midi_event_t inEvent;
	jack_midi_event_get(&inEvent, inPortBuf, i);

	const jack_midi_data_t cmd = inEvent.buffer[0] & 0xf0;

	const jack_nframes_t absTime = framesAtStart + inEvent.time;

	const jack_time_t t = jack_frames_to_time(m_client, absTime); // microseconds
	const double time = (t - m_offset) / 1000000.0;

	*m_output << time << "," << (int)inEvent.buffer[0] << "," << (int)cmd;

	switch (cmd)
	{
	case MIDI_NOTEON:
	  {
	    const jack_midi_data_t note = inEvent.buffer[1];
	    const jack_midi_data_t velocity = inEvent.buffer[2];
	    m_onTimes[note] = time;

	    *m_output << "," << (int)note << "," << (int)velocity << ",";
	    streamNoteName(*m_output, note, BEST);
	    break;
	  }
	case MIDI_NOTEOFF:
	  {
	    const jack_midi_data_t note = inEvent.buffer[1];
	    const jack_midi_data_t velocity = inEvent.buffer[2];

	    // this is a vector, not a map
	    // initialised to 0.0 anyway
	    const double onTime = m_onTimes[note];
	    const double duration = time - onTime;

	    *m_output << "," << (int)note << "," << (int)velocity << ",";
	    streamNoteName(*m_output, note, BEST);
	    *m_output << "," << onTime << "," << duration;
	    break;
	  }
	case MIDI_CC:
	  {
	    const jack_midi_data_t control = inEvent.buffer[1];
	    const jack_midi_data_t value = inEvent.buffer[2];
	    *m_output << "," << (int)control << "," << (int)value;
	    break;
	  }
	case MIDI_PC:
	  {
	    const jack_midi_data_t program = inEvent.buffer[1];
	    *m_output << "," << (int)program;
	  }
	}

	*m_output << std::endl;

      }

    }

    void DisplayHandler::shutdown()
    {
    }

  }
}
