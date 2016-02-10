#pragma once

#include "InputOutputHandler.h"
#include "MidiEvent.h"
#include "SynthParameters.h"

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
      ATTACK,                  // 0 -> 1
      SUSTAIN,                 // slow decay
      DECAY,                   // . -> 0
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
    };

    const std::string m_parametersFile;

    jack_nframes_t m_time;

    std::shared_ptr<const Parameters> m_parameters;

    // state / workspace
    std::vector<Note> m_notes;

    // table with 1 period of the note
    std::vector<double> m_samples;

    double m_attackDelta;
    double m_sustainDelta;
    double m_decayDelta;
    double m_timeMultiplier;
    double m_interpolationMultiplier;
    double m_vibratoAmplitude;

    void noteOn(const jack_midi_data_t n, const jack_nframes_t time);
    void noteOff(const jack_midi_data_t n);
    void allNotesOff();

    void processMIDIEvent(const jack_nframes_t eventCount, const jack_nframes_t localTime, const jack_nframes_t absTime, void * portBuf, jack_nframes_t & eventIndex, jack_midi_event_t & event);
    double processNotes(const jack_nframes_t absTime);

    void loadParameters();

    void generateSample(const size_t n);

    static double wave(double x, Wave type);
  };

}
