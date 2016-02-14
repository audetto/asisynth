#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ASI
{
  enum Wave
  {
    SINE,
    SAWTOOTH,
    TRIANGLE,
    SQUARE,
    NOISE
  };

  enum Pass
  {
    NONE,
    LOWPASS,
    HIGHPASS,
    BANDPASS,
    BANDSTOP
  };

  struct Harmonic
  {
    size_t mult;
    double amplitude;
    double phase;
    Wave type;
  };

  struct ADSR
  {
    double peak;
    double attackTime;
    double decayTime;
    double sustainTime;
    double releaseTime;
    double averageSize;     // low pass filter for ADSR
  };

  struct LFO
  {
    double frequency;
    double amplitude;
    std::vector<Harmonic> harmonics;
  };

  struct IIR
  {
    Pass pass;
    size_t order;
    double lower;
    double upper;
  };

  struct Parameters
  {
    size_t poliphony;
    double volume;          // note volume

    ADSR adsr;
    LFO vibrato;
    LFO tremolo;

    IIR iir;

    size_t sampleDepth;

    std::vector<Harmonic> harmonics;
  };

  std::shared_ptr<const Parameters> loadSynthParameters(const std::string & filename);

}
