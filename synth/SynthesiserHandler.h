#pragma once

#include "InputOutputHandler.h"
#include "MidiEvent.h"
#include "synth/SynthParameters.h"
#include "synth/Filter.h"

#include <jack/midiport.h>
#include <list>
#include <vector>

namespace ASI
{

  class PortMapper;

  namespace Synth
  {

    /*
      Simple synthesiser
    */
    class SynthesiserHandler : public InputOutputHandler
    {
    public:

      SynthesiserHandler(jack_client_t * client, PortMapper & mapper, const std::string & parametersFile);

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

    private:

      enum Status
      {
	ATTACK,                  // 0 -> peak
	DECAY,                   // peak -> 1
	SUSTAIN,                 // slow decay
	RELEASE,                 // . -> 0
	FORCE_RELEASE,           // this one ignores the pedal
	OFF,                     // linear ADSR = 0, smooth going to 0
	EMPTY                    // slot not used
      };

      struct Note
      {
	jack_midi_data_t n;     // MIDI number
	Real_t frequency;       // base frequency
	jack_nframes_t t0;      // start time
	Real_t phase;           // current phase
	Real_t volume;          // note volume

	Status status;
	Real_t current;         // linear ADSR
	Real_t amplitude;       // smooth ADSR

	Filter<4> filter;
      };

      struct Workspace
      {
	jack_nframes_t time;

	std::vector<Note> notes;

	// table with 1 period of the note
	std::vector<Real_t> samples;

	std::vector<Real_t> vibratoSamples;
	std::vector<Real_t> tremoloSamples;

	std::vector<Real_t> buffer;
	std::vector<Real_t> vibratoBuffer;

	jack_nframes_t sampleRate;

	bool sustain;  // the pedal

	Real_t attackDelta;
	Real_t decayDelta;
	Real_t sustainDelta;
	Real_t releaseDelta;
	Real_t actualReleaseDelta;
	Real_t timeMultiplier;
	Real_t interpolationMultiplier;

	Filter<4> filter;
      };

      const std::string m_parametersFile;

      Workspace m_work;

      std::shared_ptr<const Parameters> m_parameters;

      void noteOn(const jack_nframes_t time, const jack_midi_data_t n, const jack_midi_data_t velocity);
      void noteOff(const jack_midi_data_t n);
      void allNotesOff();

      void processMIDIEvent(const jack_nframes_t eventCount, const jack_nframes_t localTime, const jack_nframes_t absTime, void * portBuf, jack_nframes_t & eventIndex, jack_midi_event_t & event);

      void processNotes(const jack_nframes_t nframes, jack_default_audio_sample_t * output);
      void processNote(const jack_nframes_t nframes, Note & note, jack_default_audio_sample_t * output);

      void initialise();
    };

  }
}
