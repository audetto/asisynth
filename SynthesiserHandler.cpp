#include "SynthesiserHandler.h"
#include "MidiCommands.h"
#include "IIRFactory.h"

#include <cmath>
#include <iomanip>
#include <random>
#include <iostream>

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

  double wave(double x, ASI::Wave type)
  {
    switch (type)
    {
    case ASI::SINE: return std::sin(2.0 * M_PI * x);
    case ASI::SAWTOOTH: return sawtooth(x);
    case ASI::TRIANGLE: return triangle(x);
    case ASI::SQUARE: return square(x);
    case ASI::NOISE: return noise();
    default: return 0;
    }
  }

  void generateSample(const size_t size, const std::vector<ASI::Harmonic> & harmonics, std::vector<double> & samples)
  {
    samples.resize(size + 1);

    double sumOfAmplitudes = 0.0;
    for (const ASI::Harmonic & h : harmonics)
    {
      sumOfAmplitudes += h.amplitude;
    }

    const double coeff = 1.0 / size;

    for (size_t i = 0; i < size; ++i)
    {
      const double t = i * coeff;

      double total = 0.0;
      for (const ASI::Harmonic & h : harmonics)
      {
	const double frequency = 1.0 * h.mult;
	const double x = t * frequency + h.phase;
	const double amplitude = h.amplitude / sumOfAmplitudes;
	const double w = wave(x, h.type) * amplitude;
	total += w;
      }
      samples[i] = total;
    }

    // just in case the interpolation ends up in the last point
    samples.back() = samples.front();
  }

  double interpolateSample(const size_t size, const std::vector<double> & samples, const double x)
  {
    const double fx = x - size_t(x);
    const size_t pos = size_t(fx * size);
    const double w = samples[pos];
    return w;
  }

}

namespace ASI
{

  SynthesiserHandler::SynthesiserHandler(jack_client_t * client, const std::string & parametersFile)
    : InputOutputHandler(client), m_parametersFile(parametersFile)
  {
    m_inputPort = jack_port_register(m_client, "synth_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    m_outputPort = jack_port_register(m_client, "synth_out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

    m_parameters = loadSynthParameters(m_parametersFile);

    initialise();
  }

  void SynthesiserHandler::initialise()
  {
    m_work.time = 0;

    m_work.notes.resize(m_parameters->poliphony);
    for (Note & note : m_work.notes)
    {
      note.status = EMPTY;
    }

    m_work.interpolationMultiplier = 1 << m_parameters->sampleDepth;

    generateSample(m_work.interpolationMultiplier, m_parameters->harmonics, m_work.samples);
    generateSample(m_work.interpolationMultiplier, m_parameters->vibrato.harmonics, m_work.vibratoSamples);
    generateSample(m_work.interpolationMultiplier, m_parameters->tremolo.harmonics, m_work.tremoloSamples);

    // adjust vibrato sample to include amplitude multiplier
    // the amplitude in the configuration file is in Number of Semitones
    const double vibratoAmplitude = m_parameters->vibrato.amplitude * log(2.0) / 12.0;
    for (double & value : m_work.vibratoSamples)
    {
      value = exp(value * vibratoAmplitude);
    }

    // adjust tremolo sample to include amplitude multiplier and offset to 1
    for (double & value : m_work.tremoloSamples)
    {
      value = 1.0 + value * m_parameters->tremolo.amplitude;
    }
  }

  void SynthesiserHandler::processMIDIEvent(const jack_nframes_t eventCount, const jack_nframes_t localTime, const jack_nframes_t absTime, void * portBuf, jack_nframes_t & eventIndex, jack_midi_event_t & event)
  {
    while (eventIndex < eventCount && event.time == localTime)
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
    const double phaseOfLFOVibrato = absTime * m_parameters->vibrato.frequency * m_work.timeMultiplier;
    const double coeffOfLFOVibrato = interpolateSample(m_work.interpolationMultiplier, m_work.vibratoSamples, phaseOfLFOVibrato);

    const double phaseOfLFOTremolo = absTime * m_parameters->tremolo.frequency * m_work.timeMultiplier;
    const double coeffOfLFOTremolo = interpolateSample(m_work.interpolationMultiplier, m_work.tremoloSamples, phaseOfLFOTremolo);

    double total = 0.0;
    for (Note & note : m_work.notes)
    {
      switch (note.status)
      {
      case ATTACK:
	{
	  note.current += m_work.attackDelta;
	  if (note.current >= 1.0)
	  {
	    note.current = 1.0;
	    note.status = SUSTAIN;
	  }
	  break;
	}
      case SUSTAIN:
	{
	  note.current -= m_work.sustainDelta;
	  if (note.current <= 0.0)
	  {
	    note.current = 0.0;
	    note.status = OFF;
	  }
	  break;
	}
      case DECAY:
	{
	  note.current -= m_work.decayDelta;
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
      note.amplitude = (note.amplitude * m_parameters->adsr.averageSize + note.current) / (m_parameters->adsr.averageSize + 1.0);

      const double deltaPhase = note.frequency * m_work.timeMultiplier * coeffOfLFOVibrato;

      const double x = note.phase + deltaPhase;
      const double w = interpolateSample(m_work.interpolationMultiplier, m_work.samples, x);
      double value = w * note.amplitude * note.volume;
      note.phase = x;

      value = note.filter.process(value);

      total += value;
    }

    const double signal = total * coeffOfLFOTremolo;
    const double filtered = m_work.filter.process(signal);

    return filtered;
  }

  void SynthesiserHandler::process(const jack_nframes_t nframes)
  {
    void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);

    // this is float
    jack_default_audio_sample_t* outPortBuf = (jack_default_audio_sample_t *)jack_port_get_buffer(m_outputPort, nframes);

    jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

    // should we ask JACK for current time instead?
    // calling jack_last_frame_time(m_client);
    jack_nframes_t framesAtStart = m_work.time;

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

    m_work.time += nframes;
  }

  void SynthesiserHandler::sampleRate(const jack_nframes_t nframes)
  {
    m_work.attackDelta = 1.0 / m_parameters->adsr.attackTime / nframes;
    m_work.decayDelta = 1.0 / m_parameters->adsr.decayTime / nframes;
    m_work.sustainDelta = 1.0 / m_parameters->adsr.sustainTime / nframes;
    m_work.timeMultiplier = 1.0 / nframes;
    m_work.sampleRate = nframes;
  }

  void SynthesiserHandler::shutdown()
  {
  }

  void SynthesiserHandler::noteOn(const jack_midi_data_t n, const jack_nframes_t time)
  {
    const double base = std::pow(2.0, (n - 69) / 12.0) * 440.0;

    const double volume = m_parameters->volume; // * velocity

    for (Note & note : m_work.notes)
    {
      if (note.status == EMPTY)
      {
	note.n = n;
	note.frequency = base;

	note.t0 = time;
	note.phase = 0.0;
	note.volume = volume;

	note.status = ATTACK;
	note.current = 0.0;
	note.amplitude = 0.0;

	const double lower = base / m_parameters->iir.lower;
	const double upper = base * m_parameters->iir.upper;
	createButterBandPassFilter(m_parameters->iir.order, m_work.sampleRate, lower, upper, note.filter);

	return;
      }
    }

    std::cerr << "Max polyphony!" << std::endl;
  }

  void SynthesiserHandler::noteOff(const jack_midi_data_t n)
  {
    for (Note & note : m_work.notes)
    {
      if (note.n == n && note.status < DECAY)
      {
	note.status = DECAY;
      }
    }
  }

  void SynthesiserHandler::allNotesOff()
  {
    for (Note & note : m_work.notes)
    {
      if (note.status < DECAY)
      {
	note.status = DECAY;
      }
    }
  }

}
