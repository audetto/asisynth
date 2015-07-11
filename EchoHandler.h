#pragma once

#include "I_JackHandler.h"

#include <jack/midiport.h>
#include <list>

namespace ASI
{

  /*
    This class echoes all midi event coming in with a delay of lagSeconds
   */
  class EchoHandler : public I_JackHandler
  {
  public:

    EchoHandler(jack_client_t * client, double lagSeconds);

    virtual int process(jack_nframes_t nframes);

    virtual int sampleRate(jack_nframes_t nframes);

    virtual void shutdown();

  private:

    struct MidiEvent
    {
      MidiEvent(const jack_midi_data_t * data, const jack_nframes_t time);
      jack_nframes_t m_time;
      jack_midi_data_t m_data[3];
    };

    jack_client_t *m_client;
    const double m_lagSeconds;

    jack_nframes_t m_lagFrames;
    jack_port_t *m_inputPort;
    jack_port_t *m_outputPort;

    std::list<MidiEvent> m_queue;

  };

}
