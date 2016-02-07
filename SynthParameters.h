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

  struct Parameters
  {
    size_t poliphony;
    double volume;          // note volume
    double attackTime;
    double sustainTime;
    double decayTime;

    double averageSize;     // low pass filter for ADSR

    std::vector<Harmonic> harmonics;
  };

  std::shared_ptr<const Parameters> loadSynthParameters(const std::string & filename);

}
