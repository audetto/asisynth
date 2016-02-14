/*
TODO Use a better language feature like enum class
 */

#define MIDI_NOTEON 0x90
#define MIDI_NOTEOFF 0x80
#define MIDI_CC 0xB0   // or mode change
#define MIDI_PC 0xC4   // program change
#define MIDI_SYS 0xF0  // system exclusive

#define MIDI_CC_SUSTAIN 64  // pedal
#define MIDI_CC_SOSTENUTO 66 // pedal
#define MIDI_CC_ALL_SOUND_OFF 120
#define MIDI_CC_ALL_NOTES_OFF 123
