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

  void readHarmonics(const json & params, std::vector<ASI::Harmonic> & harmonics)
  {
    for (const json & h : params)
    {
      const size_t mult = h[0];
      const double amplitude = h[1];
      const double phase = h[2];
      const std::string str = h[3];

      const ASI::Wave w = strToWave(str);

      harmonics.push_back({mult, amplitude, phase, w});
    }
  }

}

namespace ASI
{

  std::shared_ptr<const Parameters> loadSynthParameters(const std::string & filename)
  {
    std::ifstream in(filename.c_str());
    const json inParams = json::parse(in);

    std::shared_ptr<Parameters> parameters(new Parameters);

    parameters->adsr.attackTime = inParams["adsr"]["attack"];
    parameters->adsr.sustainTime = inParams["adsr"]["sustain"];
    parameters->adsr.decayTime = inParams["adsr"]["decay"];

    parameters->adsr.averageSize = inParams["adsr"]["lowpass"];

    parameters->poliphony = inParams["poliphony"];
    parameters->volume = inParams["volume"];

    parameters->sampleDepth = inParams["depth"];

    readHarmonics(inParams["harmonics"], parameters->harmonics);

    parameters->vibrato.frequency = inParams["lfo"]["vibrato"]["freq"];
    parameters->vibrato.amplitude = inParams["lfo"]["vibrato"]["amplitude"];
    readHarmonics(inParams["lfo"]["vibrato"]["harmonics"], parameters->vibrato.harmonics);

    parameters->tremolo.frequency = inParams["lfo"]["tremolo"]["freq"];
    parameters->tremolo.amplitude = inParams["lfo"]["tremolo"]["amplitude"];
    readHarmonics(inParams["lfo"]["tremolo"]["harmonics"], parameters->tremolo.harmonics);

    parameters->iir.pass = BANDPASS;
    parameters->iir.order = inParams["filter"]["order"];
    parameters->iir.lower = inParams["filter"]["lower"];
    parameters->iir.upper = inParams["filter"]["upper"];

    return parameters;
  }

}
