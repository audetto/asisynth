#pragma once

#include <memory>
#include <string>
#include <vector>

namespace ASI
{
  typedef float Real_t;

  enum class Wave
  {
    SINE,
    SAWTOOTH,
    TRIANGLE,
    SQUARE,
    NOISE
  };

  enum class Pass
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
    Real_t amplitude;
    Real_t phase;
    Wave type;
  };

  struct ADSR
  {
    Real_t peak;
    Real_t attackTime;
    Real_t decayTime;
    Real_t sustainTime;
    Real_t releaseTime;
    Real_t averageSize;     // low pass filter for ADSR
  };

  struct LFO
  {
    Real_t frequency;
    Real_t amplitude;
    std::vector<Harmonic> harmonics;
  };

  struct IIR
  {
    Pass pass;
    size_t order;
    Real_t lower;
    Real_t upper;
  };

  struct Parameters
  {
    size_t poliphony;
    Real_t volume;          // note volume
    Real_t velocityPower;   // velocity ^ power * volume

    ADSR adsr;
    LFO vibrato;
    LFO tremolo;

    IIR iir;

    size_t sampleDepth;

    std::vector<Harmonic> harmonics;
  };

  std::shared_ptr<const Parameters> loadSynthParameters(const std::string & filename);

}
