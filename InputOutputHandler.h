#pragma once

#include "I_JackHandler.h"

#include <jack/midiport.h>
#include <vector>
#include <memory>

namespace ASI
{

  class CommonControls;

  class InputOutputHandler : public I_JackHandler
  {
  public:
    InputOutputHandler(const std::shared_ptr<CommonControls> & common);

  protected:

    // keep track of which notes have been generated
    // so we can cancel them
    void noteChange(const jack_midi_data_t * data);
    void allNotesOff(void * buffer, const jack_midi_data_t time);

    const std::shared_ptr<CommonControls> m_common;
    const jack_nframes_t m_sampleRate;

    jack_port_t *m_inputPort;
    jack_port_t *m_outputPort;


  private:
    std::vector<int> m_notes;

  };

}
