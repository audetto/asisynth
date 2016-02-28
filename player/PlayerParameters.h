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
      size_t velocity;
    };

    struct Bar
    {
      size_t repeat;
      std::vector<Chord> chords;
    };

    struct Melody
    {
      std::vector<Bar> bars;
      size_t tempo;
      double legatoCoeff;
    };

    std::shared_ptr<const Melody> loadPlayerMelody(const std::string & filename);

  }
}
