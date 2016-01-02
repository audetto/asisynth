#include "MidiUtils.h"

#include <vector>

namespace ASI
{
  const std::vector<std::string> namesWithSharp = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  const std::vector<std::string> namesWithFlat  = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
  const std::vector<std::string> namesWithBest  = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};

  void streamNoteName(std::ostream & s, const jack_midi_data_t note, const NoteNamePreference preference)
  {
    const int name = note % 12;
    const int octave = note / 12 - 2;

    switch (preference)
    {
    case SHARP:
      s << namesWithSharp[name];
      break;
    case FLAT:
      s << namesWithFlat[name];
      break;
    case BEST:
      s << namesWithBest[name];
      break;
    }

    s << octave;
  }
}
