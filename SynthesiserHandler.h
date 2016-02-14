#pragma once

#include "InputOutputHandler.h"
#include "MidiEvent.h"
#include "SynthParameters.h"
#include "Filter.h"

#include <jack/midiport.h>
#include <list>
#include <vector>

namespace ASI
{

  /*
    Simple synthesiser
   */
  class SynthesiserHandler : public InputOutputHandler
  {
  public:

    SynthesiserHandler(jack_client_t * client, const std::string & parametersFile);

    virtual void process(const jack_nframes_t nframes);

    virtual void sampleRate(const jack_nframes_t nframes);

    virtual void shutdown();

  private:

    enum Status
    {
      ATTACK,                  // 0 -> peak
      DECAY,                   // peak -> 1
      SUSTAIN,                 // slow decay
      RELEASE,                 // . -> 0
      OFF,                     // linear ADSR = 0, smooth going to 0
      EMPTY                    // slot not used
    };

    struct Note
    {
      jack_midi_data_t n;     // MIDI number
      double frequency;       // base frequency
      jack_nframes_t t0;      // start time
      double phase;           // current phase
      double volume;          // note volume

      Status status;
      double current;         // linear ADSR
      double amplitude;       // smooth ADSR

      Filter<4> filter;
    };

    struct Workspace
    {
      jack_nframes_t time;

      std::vector<Note> notes;

      // table with 1 period of the note
      std::vector<double> samples;

      std::vector<double> vibratoSamples;
      std::vector<double> tremoloSamples;

      jack_nframes_t sampleRate;

      double attackDelta;
      double decayDelta;
      double sustainDelta;
      double releaseDelta;
      double timeMultiplier;
      double interpolationMultiplier;

      Filter<4> filter;
    };

    const std::string m_parametersFile;

    Workspace m_work;

    std::shared_ptr<const Parameters> m_parameters;

    void noteOn(const jack_midi_data_t n, const jack_nframes_t time);
    void noteOff(const jack_midi_data_t n);
    void allNotesOff();

    void processMIDIEvent(const jack_nframes_t eventCount, const jack_nframes_t localTime, const jack_nframes_t absTime, void * portBuf, jack_nframes_t & eventIndex, jack_midi_event_t & event);
    double processNotes(const jack_nframes_t absTime);

    void initialise();
  };

}
