#include <sounds/Sounds.h>
#include <map>

namespace
{
  std::map<std::string, std::map<jack_midi_data_t, ASI::Sounds::Program>> sounds = {
    {"kdp90",
     {
       {21, {1, 121, 0}}, // Concert Grand
       {23, {1, 95, 16}}, // Concert Grand 2
       {24, {1, 121, 1}}, // Studio Grand
       {26, {2, 121, 0}}, // Modern Piano
       {28, {5, 121, 0}}, // Classic E. Piano
       {29, {6, 121, 0}}, // Modern E. Piano
       {31, {18, 121, 0}}, // Jazz Organ
       {33, {20, 121, 0}}, // Church Organ
       {35, {7, 121, 0}}, // Harpischord
       {36, {12, 121, 0}}, // Vibraphone
       {38, {49, 121, 0}}, // String Ensemble
       {40, {45, 95, 1}}, // Slow Strings
       {41, {53, 121, 0}}, // Choir
       {43, {89, 121, 0}}, // New Age Pad
       {45, {100, 121, 0}} // Atmosphere
     }
    }
  };
}

namespace ASI
{

  namespace Sounds
  {

    const std::map<jack_midi_data_t, Program> & getPianoSounds(const std::string & name)
    {
      return sounds.at(name);
    }

  }

}
