#include "handlers/transport/TransportHandler.h"
#include "sounds/Sounds.h"
#include "MidiCommands.h"
#include "CommonControls.h"

#include <cstring>

namespace ASI
{

  namespace Transport
  {
    TransportHandler::TransportHandler(const std::shared_ptr<CommonControls> & common)
      : InputOutputHandler(common), m_pedalDown(false)
    {
      jack_client_t * client = m_common->getClient();
      m_inputPort = jack_port_register(client, "transport_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
      m_outputPort = jack_port_register(client, "transport_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);
    }

    void TransportHandler::process(const jack_nframes_t nframes)
    {
      jack_client_t * client = m_common->getClient();
      const std::map<jack_midi_data_t, Sounds::Program> & sounds = m_common->getSelectedSounds();
      const jack_midi_data_t channel = m_common->getChannel();

      void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
      void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

      jack_midi_clear_buffer(outPortBuf);

      jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

      for (size_t i = 0; i < eventCount; ++i)
      {
	jack_midi_event_t inEvent;
	jack_midi_event_get(&inEvent, inPortBuf, i);

	const jack_midi_data_t cmd = inEvent.buffer[0] & 0xf0;

	switch (cmd)
	{
	case MIDI_CC:
	  {
	    const jack_midi_data_t control = inEvent.buffer[1];
	    if (control == MIDI_CC_SOSTENUTO)
	    {
	      const jack_midi_data_t value = inEvent.buffer[2];

	      const bool newPedalDown = value >= 64;
	      if (newPedalDown != m_pedalDown)
	      {
		// it has changed
		if (newPedalDown)
		{
		  // stop everything, and restart at zero for now
		  jack_transport_stop(client);
		  jack_position_t pos;
		  memset(&pos, 0, sizeof(pos));
		  pos.frame = 0;
		  jack_transport_reposition(client, &pos);
		}
		else
		{
		  // it has just been activated
		  jack_transport_start(client);
		}
		m_pedalDown = newPedalDown;
	      }
	    }
	    break;
	  }
	case MIDI_NOTEON:
	  {
	    const jack_midi_data_t velocity = inEvent.buffer[2];
	    if (m_pedalDown && velocity > 0)
	    {
	      const jack_midi_data_t note = inEvent.buffer[1];
	      if (sounds.count(note) > 0)
	      {
		const Sounds::Program & program = sounds.at(note);
		jack_midi_data_t data[3];
		data[0] = MIDI_CC | (channel - 1);
		data[1] = MIDI_CC_MSB;
		data[2] = program.msb;
		jack_midi_event_write(outPortBuf, inEvent.time, data, 3);

		// repeated for clarity
		data[0] = MIDI_CC | (channel - 1);
		data[1] = MIDI_CC_LSB;
		data[2] = program.lsb;
		jack_midi_event_write(outPortBuf, inEvent.time, data, 3);

		data[0] = MIDI_PC | (channel - 1);
		data[1] = program.number - 1;
		jack_midi_event_write(outPortBuf, inEvent.time, data, 2);
	      }
	    }
	    break;
	  }
	}
      }

    }

    void TransportHandler::shutdown()
    {
    }

  }
}
