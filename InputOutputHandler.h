#pragma once

#include "I_JackHandler.h"

#include <jack/midiport.h>
#include <vector>

namespace ASI
{

  class InputOutputHandler : public I_JackHandler
  {
  public:
    InputOutputHandler(jack_client_t * client);

  protected:

    // keep track of which notes have been generated
    // so we can cancel them
    void noteChange(const jack_midi_data_t * data);
    void allNotesOff(void * buffer, const jack_midi_data_t time);

    jack_client_t *m_client;
    jack_port_t *m_inputPort;
    jack_port_t *m_outputPort;

    const jack_nframes_t m_sampleRate;

  private:
    std::vector<int> m_notes;

  };

}
