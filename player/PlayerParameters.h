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
      size_t period;
      std::vector<std::pair<size_t, size_t> > velocity; // reminder, velocity
    };

    std::shared_ptr<const Melody> loadPlayerMelody(const std::string & filename);

  }
}
