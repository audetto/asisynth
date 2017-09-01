#include "player/PlayerParameters.h"

#include <vector>
#include <string>
#include <memory>

#include <json.hpp>
#include <fstream>

using json = nlohmann::json;

namespace
{
  void processValues(const json & values, std::vector<ASI::Player::Chord> & chords);
  void processLoop(const json & values, std::vector<ASI::Player::Chord> & chords);
  void processChord(const json & values, std::vector<ASI::Player::Chord> & chords);

  void processChord(const json & data, std::vector<ASI::Player::Chord> & chords)
  {
    ASI::Player::Chord chord;
    chord.duration = data["duration"];

    for (const json & n : data["notes"])
    {
      const size_t midi = n;
      chord.notes.push_back(midi);
    }

    chords.push_back(chord);
  }

  void processLoop(const json & data, std::vector<ASI::Player::Chord> & chords)
  {
    const size_t counter = data["repeat"];
    std::vector<ASI::Player::Chord> values;
    processValues(data["values"], values);

    for (size_t i = 0; i < counter; ++i)
    {
      chords.insert(chords.end(), values.begin(), values.end());
    }
  }

  void processValues(const json & values, std::vector<ASI::Player::Chord> & chords)
  {
    for (const json & v : values)
    {
      if (v.find("chord") != v.end())
      {
	processChord(v["chord"], chords);
      }
      else if (v.find("loop") != v.end())
      {
	processLoop(v["loop"], chords);
      }
    }
  }

  void processVelocity(const json & data, std::vector<size_t> & velocity)
  {
    for (const json & k : data)
    {
      velocity.push_back(k);
    }
  }
}

namespace ASI
{
  namespace Player
  {

    std::shared_ptr<const Melody> loadPlayerMelody(const std::string & filename)
    {
      std::ifstream in(filename.c_str());
      const json inParams = json::parse(in);

      std::shared_ptr<Melody> melody(new Melody);

      melody->tempo = inParams["tempo"];
      melody->legatoCoeff = inParams["legato"];

      processVelocity(inParams["velocity"], melody->velocity);

      processValues(inParams["values"], melody->chords);

      return melody;
    }

  }

}
