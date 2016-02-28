#include "PlayerParameters.h"

#include <vector>
#include <string>
#include <memory>

#include <json.hpp>
#include <fstream>

using json = nlohmann::json;

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

      for (const json & b : inParams["bars"])
      {
	Bar bar;
	bar.repeat = b["repeat"];
	for (const json & c : b["chords"])
	{
	  Chord chord;
	  chord.duration = c["duration"];
	  chord.velocity = c["velocity"];

	  for (const json & n : c["notes"])
	  {
	    const size_t midi = n;
	    chord.notes.push_back(midi);
	  }

	  bar.chords.push_back(chord);
	}
	melody->bars.push_back(bar);
      }

      return melody;
    }

  }

}
