#pragma once

#include "InputOutputHandler.h"
#include "MidiEvent.h"

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

    SynthesiserHandler(jack_client_t * client);

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

    enum Wave
    {
      SINE,
      SAWTOOTH,
      TRIANGLE,
      SQUARE
    };

    struct Note
    {
      jack_midi_data_t n;     // MIDI number
      double frequency;       // base frequency
      jack_nframes_t t0;      // start time
      double volume;          // note volume

      Status status;
      double current;         // linear ADSR
      double amplitude;       // smooth ADSR
    };

    struct Harmonic
    {
      double mult;
      double amplitude;
      double phase;
      Wave type;
    };

    struct Parameters
    {
      double volume;          // note volume
      double attackTime;
      double sustainTime;
      double decayTime;

      double averageSize;     // low pass filter for ADSR
    };

    jack_nframes_t myTime;

    Parameters myParameters;

    std::vector<Note> myNotes;

    std::vector<Harmonic> myHarmonics;

    double myAttackDelta;
    double mySustainDelta;
    double myDecayDelta;
    double myTimeMultiplier;

    void addNote(const jack_midi_data_t n, const jack_nframes_t time);
    void removeNote(const jack_midi_data_t n);

    static double wave(double x, Wave type);
  };

}
