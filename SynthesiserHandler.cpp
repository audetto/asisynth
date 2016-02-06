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

    m_parameters.harmonics = {
      {1, 1.00, 0.0, SINE},
      {1, 1.00, 0.0, TRIANGLE},
      {2, 0.10, 0.0, SINE},
      {2, 0.10, 0.0, TRIANGLE},
      {3, 0.05, 0.0, SINE},
      {3, 0.05, 0.0, TRIANGLE},
    };

    m_parameters.volume = 0.2;
    m_parameters.attackTime = 0.05;
    m_parameters.sustainTime = 5.0;
    m_parameters.decayTime = 0.1;
    m_parameters.averageSize = 10.0;

    m_time = 0;

    m_notes.resize(32);
    for (Note & note : m_notes)
    {
      note.status = EMPTY;
    }

  }

  void SynthesiserHandler::processMIDIEvent(const jack_nframes_t eventCount, const jack_nframes_t localTime, const jack_nframes_t absTime, void * portBuf, jack_nframes_t & eventIndex, jack_midi_event_t & event)
  {
    if (eventIndex < eventCount && event.time == localTime)
    {
      const jack_midi_data_t cmd = event.buffer[0] & 0xf0;
      const jack_midi_data_t n = event.buffer[1];
      const jack_midi_data_t velocity = event.buffer[2];

      switch (cmd)
      {
      case MIDI_NOTEON:
	{
	  if (velocity == 0)
	  {
	    noteOff(n);
	  }
	  else
	  {
	    noteOn(n, absTime);
	  }
	  break;
	}
      case MIDI_NOTEOFF:
	{
	  noteOff(n);
	  break;
	}
      case MIDI_CC:
	{
	  if (n == 123)
	  {
	    allNotesOff();
	  }
	  break;
	}
      }

      ++eventIndex;
      if (eventIndex < eventCount)
      {
	jack_midi_event_get(&event, portBuf, eventIndex);
      }
    }

  }

  double SynthesiserHandler::processNotes(const jack_nframes_t absTime)
  {
    double total = 0.0;
    for (Note & note : m_notes)
    {
      switch (note.status)
      {
      case ATTACK:
	{
	  note.current += m_attackDelta;
	  if (note.current >= 1.0)
	  {
	    note.current = 1.0;
	    note.status = SUSTAIN;
	  }
	  break;
	}
      case SUSTAIN:
	{
	  note.current -= m_sustainDelta;
	  if (note.current <= 0.0)
	  {
	    note.current = 0.0;
	    note.status = OFF;
	  }
	  break;
	}
      case DECAY:
	{
	  note.current -= m_decayDelta;
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
      note.amplitude = (note.amplitude * m_parameters.averageSize + note.current) / (m_parameters.averageSize + 1.0);

      const jack_nframes_t tInFrames = absTime - note.t0;
      const double t = tInFrames * m_timeMultiplier;
      const double x = note.frequency * t;

      const double fx = x - size_t(x);
      const size_t pos = size_t(fx * m_interpolationMultiplier);
      const double w = m_samples[pos] * note.amplitude * note.volume;
      total += w;

    }

    return total;
  }

  void SynthesiserHandler::process(const jack_nframes_t nframes)
  {
    void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);

    // this is float
    jack_default_audio_sample_t* outPortBuf = (jack_default_audio_sample_t *)jack_port_get_buffer(m_outputPort, nframes);

    jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

    // should we ask JACK for current time instead?
    // calling jack_last_frame_time(m_client);
    jack_nframes_t framesAtStart = m_time;

    jack_nframes_t eventIndex = 0;

    jack_midi_event_t inEvent;
    if (eventIndex < eventCount)
    {
      jack_midi_event_get(&inEvent, inPortBuf, eventIndex);
    }

    for(size_t i = 0; i < nframes; ++i)
    {
      const jack_nframes_t absTime = framesAtStart + i;

      processMIDIEvent(eventCount, i, absTime, inPortBuf, eventIndex, inEvent);
      const double w = processNotes(absTime);

      outPortBuf[i] = w;
    }

    m_time += nframes;
  }

  void SynthesiserHandler::sampleRate(const jack_nframes_t nframes)
  {
    m_attackDelta = 1.0 / m_parameters.attackTime / nframes;
    m_decayDelta = 1.0 / m_parameters.decayTime / nframes;
    m_sustainDelta = 1.0 / m_parameters.sustainTime / nframes;
    m_timeMultiplier = 1.0 / nframes;

    generateSample(nframes);
  }

  void SynthesiserHandler::shutdown()
  {
  }

  void SynthesiserHandler::noteOn(const jack_midi_data_t n, const jack_nframes_t time)
  {
    const double base = std::pow(2.0, (n - 69) / 12.0) * 440.0;

    const double volume = m_parameters.volume; // * velocity

    for (Note & note : m_notes)
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

  void SynthesiserHandler::noteOff(const jack_midi_data_t n)
  {
    for (Note & note : m_notes)
    {
      if (note.n == n)
      {
	note.status = DECAY;
      }
    }
  }

  void SynthesiserHandler::allNotesOff()
  {
    for (Note & note : m_notes)
    {
      note.status = DECAY;
    }
  }

  void SynthesiserHandler::generateSample(const size_t n)
  {
    m_samples.resize(n + 1);

    m_interpolationMultiplier = n;

    double sumOfAmplitudes = 0.0;
    for (const Harmonic & h : m_parameters.harmonics)
    {
      sumOfAmplitudes += h.amplitude;
    }

    const double coeff = 1.0 / n;

    for (size_t i = 0; i < n; ++i)
    {
      const double t = i * coeff;

      double total = 0.0;
      for (const Harmonic & h : m_parameters.harmonics)
      {
	const double frequency = 1.0 * h.mult;
	const double x = t * frequency + h.phase;
	const double amplitude = h.amplitude / sumOfAmplitudes;
	const double w = wave(x, h.type) * amplitude;
	total += w;
      }
      m_samples[i] = total;
    }

    // just in case the interpolation ends up in the last point
    m_samples.back() = m_samples.front();
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
