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

  struct Harmonic
  {
    size_t mult;
    double amplitude;
    double phase;
    Wave type;
  };

  struct ADSR
  {
    double attackTime;
    double sustainTime;
    double decayTime;
    double averageSize;     // low pass filter for ADSR
  };

  struct LFO
  {
    double frequency;
    double amplitude;
  };

  struct Parameters
  {
    size_t poliphony;
    double volume;          // note volume

    ADSR adsr;
    LFO vibrato;
    LFO tremolo;

    size_t sampleDepth;

    std::vector<Harmonic> harmonics;
  };

  std::shared_ptr<const Parameters> loadSynthParameters(const std::string & filename);

}
