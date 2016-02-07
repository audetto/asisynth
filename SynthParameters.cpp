#include "SynthParameters.h"

#include <json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace
{
  ASI::Wave strToWave(const std::string & s)
  {
    if (s == "sine")
      return ASI::SINE;

    if (s == "square")
      return ASI::SQUARE;

    if (s == "sawtooth")
      return ASI::SAWTOOTH;

    if (s == "triangle")
      return ASI::TRIANGLE;

    if (s == "noise")
      return ASI::NOISE;

    throw std::runtime_error("Unknown wave type");
  }
}

namespace ASI
{

  std::shared_ptr<const Parameters> loadSynthParameters(const std::string & filename)
  {
    std::ifstream in(filename.c_str());
    const json inParams = json::parse(in);

    std::shared_ptr<Parameters> parameters(new Parameters);

    parameters->attackTime = inParams["adsr"]["attack"];
    parameters->sustainTime = inParams["adsr"]["sustain"];
    parameters->decayTime = inParams["adsr"]["decay"];

    parameters->averageSize = inParams["adsr"]["lowpass"];

    parameters->poliphony = inParams["poliphony"];
    parameters->volume = inParams["volume"];

    const json & hs = inParams["harmonics"];

    for (const json & h : hs)
    {
      const size_t mult = h[0];
      const double amplitude = h[1];
      const double phase = h[2];
      const std::string str = h[3];

      const Wave w = strToWave(str);

      parameters->harmonics.push_back({mult, amplitude, phase, w});
    }

    return parameters;
  }

}
