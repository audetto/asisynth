#include "SynthesiserHandler.h"
#include "MidiCommands.h"
#include "IIRFactory.h"

#include <cmath>
#include <cstring>
#include <iomanip>
#include <random>
#include <iostream>

namespace
{
  using ASI::Real_t;

  Real_t sawtooth(Real_t x)
  {
    // period 1, range [-1, 1]
    return 2.0 * (x - std::floor(0.5 + x));
  }

  Real_t triangle(Real_t x)
  {
    // period 1, range [-1, 1]
    return 2.0 * std::abs(sawtooth(x)) - 1.0;
  }

  Real_t square(Real_t x)
  {
    return x - std::floor(x) > 0.5 ? 1.0 : -1.0;
  }

  Real_t noise()
  {
    static std::default_random_engine generator;
    static std::uniform_real_distribution<Real_t> distribution(-1.0, 1.0);

    return distribution(generator);
  }

  Real_t wave(Real_t x, ASI::Wave type)
  {
    switch (type)
    {
    case ASI::Wave::SINE: return std::sin(2.0 * M_PI * x);
    case ASI::Wave::SAWTOOTH: return sawtooth(x);
    case ASI::Wave::TRIANGLE: return triangle(x);
    case ASI::Wave::SQUARE: return square(x);
    case ASI::Wave::NOISE: return noise();
    default: return 0;
    }
  }

  void generateSample(const size_t size, const std::vector<ASI::Harmonic> & harmonics, std::vector<Real_t> & samples)
  {
    samples.resize(size + 1);

    Real_t sumOfAmplitudes = 0.0;
    for (const ASI::Harmonic & h : harmonics)
    {
      sumOfAmplitudes += h.amplitude;
    }

    const Real_t coeff = 1.0 / size;

    for (size_t i = 0; i < size; ++i)
    {
      const Real_t t = i * coeff;

      Real_t total = 0.0;
      for (const ASI::Harmonic & h : harmonics)
      {
	const Real_t frequency = 1.0 * h.mult;
	const Real_t x = t * frequency + h.phase;
	const Real_t amplitude = h.amplitude / sumOfAmplitudes;
	const Real_t w = wave(x, h.type) * amplitude;
	total += w;
      }
      samples[i] = total;
    }

    // just in case the interpolation ends up in the last point
    samples.back() = samples.front();
  }

  Real_t interpolateSample(const size_t size, const std::vector<Real_t> & samples, const Real_t x)
  {
    const Real_t fx = x - size_t(x);
    const size_t pos = size_t(fx * size);
    const Real_t w = samples[pos];
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
    m_work.sustain = false;

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
    const Real_t vibratoAmplitude = m_parameters->vibrato.amplitude * log(2.0) / 12.0;
    for (Real_t & value : m_work.vibratoSamples)
    {
      value = exp(value * vibratoAmplitude);
    }

    // adjust tremolo sample to include amplitude multiplier and offset to 1
    for (Real_t & value : m_work.tremoloSamples)
    {
      value = 1.0 + value * m_parameters->tremolo.amplitude;
    }

    // so we do not allocate during "process callback"
    m_work.buffer.resize(8192);
    m_work.vibratoBuffer.resize(8192);
  }

  void SynthesiserHandler::processMIDIEvent(const jack_nframes_t eventCount, const jack_nframes_t localTime, const jack_nframes_t absTime, void * portBuf, jack_nframes_t & eventIndex, jack_midi_event_t & event)
  {
    while (eventIndex < eventCount && event.time == localTime)
    {
      const jack_midi_data_t cmd = event.buffer[0] & 0xf0;
      const jack_midi_data_t n1 = event.buffer[1];
      const jack_midi_data_t n2 = event.buffer[2];

      switch (cmd)
      {
      case MIDI_NOTEON:
	{
	  if (n2 == 0)
	  {
	    noteOff(n1);
	  }
	  else
	  {
	    noteOn(absTime, n1, n2);
	  }
	  break;
	}
      case MIDI_NOTEOFF:
	{
	  noteOff(n1);
	  break;
	}
      case MIDI_CC:
	{
	  switch (n1)
	  {
	  case MIDI_CC_ALL_SOUND_OFF:
	  case MIDI_CC_ALL_NOTES_OFF:
	    {
	      allNotesOff();
	      break;
	    }
	  case MIDI_CC_SUSTAIN:
	    {
	      m_work.sustain = n2 >= 64;
	      // if the sustain pedal is pressed
	      // RELEASE behaves the same as SUSTAIN
	      // we could work on the status
	      // but this uses less "if"
	      m_work.actualReleaseDelta = m_work.sustain ? m_work.sustainDelta : m_work.releaseDelta;

	      break;
	    }
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

  void SynthesiserHandler::processNote(const jack_nframes_t nframes, Note & note, jack_default_audio_sample_t * output)
  {
    if (note.status == EMPTY)
    {
      return;
    }

    for (size_t i = 0; i < nframes; ++i)
    {
      switch (note.status)
      {
      case ATTACK:
	{
	  note.current += m_work.attackDelta;
	  if (note.current >= m_parameters->adsr.peak)
	  {
	    note.current = m_parameters->adsr.peak;
	    note.status = DECAY;
	  }
	  break;
	}
      case DECAY:
	{
	  note.current -= m_work.decayDelta;
	  if (note.current <= 1.0)
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
      case RELEASE:
	{
	  // same as SUSTAIN if the pedal is down
	  note.current -= m_work.actualReleaseDelta;
	  if (note.current <= 0.0)
	  {
	    note.current = 0.0;
	    note.status = OFF;
	  }
	  break;
	}
      case FORCE_RELEASE:
	{
	  // same as RELEASE but the pedal is ignored
	  note.current -= m_work.releaseDelta;
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
	  m_work.buffer[i] = 0.0;
	  continue;
	}
      };

      // this is a low pass filter to smooth the ADSR
      // is it needed?
      note.amplitude = (note.amplitude * m_parameters->adsr.averageSize + note.current) / (m_parameters->adsr.averageSize + 1.0);

      const Real_t w = interpolateSample(m_work.interpolationMultiplier, m_work.samples, note.phase);
      const Real_t value = w * note.amplitude * note.volume;
      m_work.buffer[i] = value;

      const Real_t deltaPhase = note.frequency * m_work.timeMultiplier * m_work.vibratoBuffer[i];
      note.phase = note.phase + deltaPhase;
      if (note.phase >= 1.0)
      {
	note.phase -= 1.0;
      }
    }

    note.filter.process(m_work.buffer.data(), nframes);

    for (size_t i = 0; i < nframes; ++i)
    {
      output[i] += m_work.buffer[i];
    }
  }

  void SynthesiserHandler::processNotes(const jack_nframes_t nframes, jack_default_audio_sample_t * output)
  {
    for (size_t i = 0; i < nframes; ++i)
    {
      const jack_nframes_t absTime = m_work.time + i;
      const Real_t phaseOfLFOVibrato = absTime * m_parameters->vibrato.frequency * m_work.timeMultiplier;
      const Real_t coeffOfLFOVibrato = interpolateSample(m_work.interpolationMultiplier, m_work.vibratoSamples, phaseOfLFOVibrato);

      m_work.vibratoBuffer[i] = coeffOfLFOVibrato;
    }

    for (Note & note : m_work.notes)
    {
      processNote(nframes, note, output);
    }

    for (size_t i = 0; i < nframes; ++i)
    {
      const jack_nframes_t absTime = m_work.time + i;
      const Real_t phaseOfLFOTremolo = absTime * m_parameters->tremolo.frequency * m_work.timeMultiplier;
      const Real_t coeffOfLFOTremolo = interpolateSample(m_work.interpolationMultiplier, m_work.tremoloSamples, phaseOfLFOTremolo);

      output[i] *= coeffOfLFOTremolo;
    }

    m_work.time += nframes;
  }

  void SynthesiserHandler::process(const jack_nframes_t nframes)
  {
    void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);

    // this is Real_t
    jack_default_audio_sample_t* output = (jack_default_audio_sample_t *)jack_port_get_buffer(m_outputPort, nframes);

    memset(output, 0, sizeof(jack_default_audio_sample_t) * nframes);

    jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

    jack_nframes_t eventIndex = 0;

    jack_midi_event_t inEvent;
    if (eventIndex < eventCount)
    {
      jack_midi_event_get(&inEvent, inPortBuf, eventIndex);
    }

    jack_nframes_t position = 0;
    while (position < nframes)
    {
      jack_nframes_t toProcess;
      if (eventIndex < eventCount)
      {
	toProcess = inEvent.time - position;
      }
      else
      {
	toProcess = nframes - position;
      }
      processNotes(toProcess, output + position);
      position += toProcess;
      processMIDIEvent(eventCount, position, m_work.time, inPortBuf, eventIndex, inEvent);
    }

    m_work.filter.process(output, nframes);
  }

  void SynthesiserHandler::sampleRate(const jack_nframes_t nframes)
  {
    m_work.attackDelta = m_parameters->adsr.peak / m_parameters->adsr.attackTime / nframes;
    m_work.decayDelta = (m_parameters->adsr.peak - 1.0) / m_parameters->adsr.decayTime / nframes;
    m_work.sustainDelta = 1.0 / m_parameters->adsr.sustainTime / nframes;
    m_work.releaseDelta = 1.0 / m_parameters->adsr.releaseTime / nframes;
    m_work.actualReleaseDelta = m_work.sustain ? m_work.sustainDelta : m_work.releaseDelta;
    m_work.timeMultiplier = 1.0 / nframes;
    m_work.sampleRate = nframes;
  }

  void SynthesiserHandler::shutdown()
  {
  }

  void SynthesiserHandler::noteOn(const jack_nframes_t time, const jack_midi_data_t n, const jack_midi_data_t velocity)
  {
    const Real_t base = std::pow(2.0, (n - 69) / 12.0) * 440.0;

    const Real_t coeff = pow(velocity / 127.0, m_parameters->velocityPower);
    const Real_t volume = m_parameters->volume * coeff;

    Note * newNote = nullptr;

    for (Note & note : m_work.notes)
    {
      if (note.status == EMPTY)
      {
	newNote = &note;
      }
      else
      {
	if (note.n == n)
	{
	  // reuse existing note
	  // rather than playing 2 notes at the same frequency
	  note.status = ATTACK;
	  note.volume = volume;
	  return;
	}
      }
    }

    if (newNote)
    {
      Note & note = *newNote;
      note.n = n;
      note.frequency = base;

      note.t0 = time;
      note.phase = 0.0;
      note.volume = volume;

      note.status = ATTACK;
      note.current = 0.0;
      note.amplitude = 0.0;

      const Real_t lower = base / m_parameters->iir.lower;
      const Real_t upper = base * m_parameters->iir.upper;
      createFilter(m_parameters->iir.pass, m_parameters->iir.order, m_work.sampleRate, lower, upper, note.filter);
      return;
    }

    std::cerr << "Max polyphony!" << std::endl;
  }

  void SynthesiserHandler::noteOff(const jack_midi_data_t n)
  {
    for (Note & note : m_work.notes)
    {
      if (note.n == n && note.status < RELEASE)
      {
	note.status = RELEASE;
      }
    }
  }

  void SynthesiserHandler::allNotesOff()
  {
    for (Note & note : m_work.notes)
    {
      if (note.status < FORCE_RELEASE)
      {
	note.status = FORCE_RELEASE;
      }
    }
  }

}
