#include "SynthesiserHandler.h"
#include "MidiCommands.h"

#include <cmath>
#include <iomanip>
#include <random>

namespace
{

  double sawtooth(double x)
  {
    // period 1, range [-1, 1]
    return 2.0 * (x - std::floor(0.5 + x));
  }

  double triangle(double x)
  {
    // period 1, range [-1, 1]
    return 2.0 * std::abs(sawtooth(x)) - 1.0;
  }

  double square(double x)
  {
    return x - std::floor(x) > 0.5 ? 1.0 : -1.0;
  }

  double noise()
  {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<double> distribution(-1.0, 1.0);

    return distribution(generator);
  }

}

namespace ASI
{

  SynthesiserHandler::SynthesiserHandler(jack_client_t * client)
    : InputOutputHandler(client)
  {
    m_inputPort = jack_port_register(m_client, "synth_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    m_outputPort = jack_port_register(m_client, "synth_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    myParameters.harmonics = {
      {1.0, 1.00, 0.0, SINE},
      {1.0, 1.00, 0.0, TRIANGLE},
      {2.0, 0.10, 0.0, SINE},
      {2.0, 0.10, 0.0, TRIANGLE},
      {3.0, 0.05, 0.0, SINE},
      {3.0, 0.05, 0.0, TRIANGLE},
    };

    myParameters.volume = 0.2;
    myParameters.attackTime = 0.05;
    myParameters.sustainTime = 5.0;
    myParameters.decayTime = 0.1;
    myParameters.averageSize = 10.0;

    myTime = 0;

    myNotes.resize(32);
    for (Note & note : myNotes)
    {
      note.status = EMPTY;
    }

  }

  void SynthesiserHandler::process(const jack_nframes_t nframes)
  {
    void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);

    // this is float
    jack_default_audio_sample_t* outPortBuf = (jack_default_audio_sample_t *)jack_port_get_buffer(m_outputPort, nframes);

    jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

    // should we ask JACK for current time instead?
    // calling jack_last_frame_time(m_client);
    jack_nframes_t framesAtStart = myTime;

    jack_nframes_t eventIndex = 0;

    jack_midi_event_t inEvent;
    if (eventIndex < eventCount)
    {
      jack_midi_event_get(&inEvent, inPortBuf, eventIndex);
    }

    for(size_t i = 0; i < nframes; ++i)
    {
      const jack_nframes_t time = framesAtStart + i;

      if (eventIndex < eventCount && inEvent.time == i)
      {
	const jack_midi_data_t cmd = inEvent.buffer[0] & 0xf0;
	const jack_midi_data_t n = inEvent.buffer[1];
	const jack_midi_data_t velocity = inEvent.buffer[2];

	switch (cmd)
	{
	case MIDI_NOTEON:
	  {
	    if (velocity == 0)
	    {
	      removeNote(n);
	    }
	    else
	    {
	      addNote(n, time);
	    }
	    break;
	  }
	case MIDI_NOTEOFF:
	  {
	    removeNote(n);
	    break;
	  }
	case MIDI_CC:
	  {
	    if (n == 123)
	    {
	      removeAllNotes();
	    }
	    break;
	  }
	}

	++eventIndex;
	if (eventIndex < eventCount)
	{
	  jack_midi_event_get(&inEvent, inPortBuf, eventIndex);
	}
      }

      double total = 0.0;
      for (Note & note : myNotes)
      {
	switch (note.status)
	{
	case ATTACK:
	  {
	    note.current += myAttackDelta;
	    if (note.current >= 1.0)
	    {
	      note.current = 1.0;
	      note.status = SUSTAIN;
	    }
	    break;
	  }
	case SUSTAIN:
	  {
	    note.current -= mySustainDelta;
	    if (note.current <= 0.0)
	    {
	      note.current = 0.0;
	      note.status = OFF;
	    }
	    break;
	  }
	case DECAY:
	  {
	    note.current -= myDecayDelta;
	    if (note.current <= 0.0)
	    {
	      note.current = 0.0;
	      note.status = OFF;
	    }
	    break;
	  }
	case OFF:
	  {
	    if (note.amplitude <= 0.00000001)
	    {
	      note.status = EMPTY;
	    }
	    break;
	  }
	case EMPTY:
	  {
	    continue;
	  }
	};

	// this is a low pass filter to smooth the ADSR
	// is it needed?
	note.amplitude = (note.amplitude * myParameters.averageSize + note.current) / (myParameters.averageSize + 1.0);

	const jack_nframes_t tInFrames = time - note.t0;
	const double t = tInFrames * myTimeMultiplier;
	const double x = note.frequency * t;

	const double fx = x - size_t(x);
	const size_t pos = size_t(fx * myInterpolationMultiplier);
	const double w = mySamples[pos] * note.amplitude * note.volume;
	total += w;

      }
      outPortBuf[i] = total;
    }

    myTime += nframes;
  }

  void SynthesiserHandler::sampleRate(const jack_nframes_t nframes)
  {
    myAttackDelta = 1.0 / myParameters.attackTime / nframes;
    myDecayDelta = 1.0 / myParameters.decayTime / nframes;
    mySustainDelta = 1.0 / myParameters.sustainTime / nframes;
    myTimeMultiplier = 1.0 / nframes;

    generateSample(nframes);
  }

  void SynthesiserHandler::shutdown()
  {
  }

  void SynthesiserHandler::addNote(const jack_midi_data_t n, const jack_nframes_t time)
  {
    if (myParameters.harmonics.empty())
    {
      return;
    }

    const double base = std::pow(2.0, (n - 69) / 12.0) * 440.0;

    const double volume = myParameters.volume; // * velocity

    for (Note & note : myNotes)
    {
      if (note.status == EMPTY)
      {
	note.n = n;
	note.frequency = base;

	note.t0 = time;
	note.volume = volume;

	note.status = ATTACK;
	note.current = 0.0;
	note.amplitude = 0.0;
	return;
      }
    }
  }

  void SynthesiserHandler::removeNote(const jack_midi_data_t n)
  {
    for (Note & note : myNotes)
    {
      if (note.n == n)
      {
	note.status = DECAY;
      }
    }
  }

  void SynthesiserHandler::removeAllNotes()
  {
    for (Note & note : myNotes)
    {
      note.status = DECAY;
    }
  }

  void SynthesiserHandler::generateSample(const size_t n)
  {
    mySamples.resize(n + 1);

    myInterpolationMultiplier = n;

    double sumOfAmplitudes = 0.0;
    for (const Harmonic & h : myParameters.harmonics)
    {
      sumOfAmplitudes += h.amplitude;
    }

    const double coeff = 1.0 / n;

    for (size_t i = 0; i < n; ++i)
    {
      const double t = i * coeff;

      double total = 0.0;
      for (const Harmonic & h : myParameters.harmonics)
      {
	const double frequency = 1.0 * h.mult;
	const double x = t * frequency + h.phase;
	const double amplitude = h.amplitude / sumOfAmplitudes;
	const double w = wave(x, h.type) * amplitude;
	total += w;
      }
      mySamples[i] = total;
    }

    // just in case the interpolation ends up in the last point
    mySamples.back() = mySamples.front();
  }


  double SynthesiserHandler::wave(double x, Wave type)
  {
    switch (type)
    {
    case SINE: return std::sin(2.0 * M_PI * x);
    case SAWTOOTH: return sawtooth(x);
    case TRIANGLE: return triangle(x);
    case SQUARE: return square(x);
    case NOISE: return noise();
    }
  }

}
