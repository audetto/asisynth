#include <vector>
#include <string>
#include <memory>

namespace ASI
{
  namespace Player
  {

    struct Chord
    {
      std::vector<size_t> notes;
      size_t duration;
    };

    struct Melody
    {
      std::vector<Chord> chords;
      size_t tempo;
      double legatoCoeff;
      std::vector<size_t> velocity; // in a loop
    };

    std::shared_ptr<const Melody> loadPlayerMelody(const std::string & filename);

  }
}
