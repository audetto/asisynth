#include "chords/ChordPlayerHandler.h"
#include "MidiCommands.h"
#include "MidiUtils.h"
#include "PortMapper.h"

#include <fstream>
#include <iostream>
#include <regex>
#include <map>
#include <boost/algorithm/string.hpp>

namespace
{
  const std::map<char, int> MIDI = {{'A', 9}, {'B', 11}, {'C', 0}, {'D', 2}, {'E', 4}, {'F', 5}, {'G', 7}};

  const std::map<std::string, int> alterations = {{"bb", -2}, {"b", -1}, {"", 0}, {"#", 1}, {"##", 2}};

  const std::map<std::string, std::vector<int> > modes = {
    {"maj",  {0, 4, 7}},
    {"dom7", {0, 4, 7, 10}},
    {"min",  {0, 3, 7}},
    {"min7", {0, 3, 7, 10}},
    {"sus4", {0, 5, 7}}
  };

  int getRootNote(const std::string & s, const std::string & alt, const int root)
  {
    const char c = s[0];
    int pos = root + MIDI.at(c);

    pos += alterations.at(alt);

    return pos;
  }

  jack_midi_data_t getNoteAndOctave(const std::string & s)
  {
    std::regex e("^([CDEFGAB])(#|##|b|bb)?([0-8])$");
    std::smatch m;

    const bool ok = std::regex_search(s, m, e) && (m.size() == 4);

    if (!ok)
    {
      const std::string message = "Failed to parse: " + s;
      throw std::runtime_error(message);
    }

    const std::string note = m[1].str();
    const std::string alt = m[2].str();
    const std::string oct = m[3].str();

    const int octave = std::stoi(oct);
    // convention is MIDDLE C = C3
    const int root = (octave + 2) * 12;

    const int pos = getRootNote(note, alt, root);

    return pos;
  }

  void getAllNotes(const int pos, const std::string & mode, std::vector<jack_midi_data_t> & notes)
  {
    const std::vector<int> & offsets = modes.at(mode);
    for (auto o : offsets)
    {
      notes.push_back(pos + o);
    }
  }

  void createChord(const std::string & s, std::vector<jack_midi_data_t> & notes)
  {
    std::regex e("^([CDEFGAB])(#|##|b|bb)?(maj|dom7|min|min7|sus4)(?:/([CDEFGAB])(#|##|b|bb)?)?$");
    std::smatch m;

    const int root = 48; // for the chords

    const bool ok = std::regex_search(s, m, e) && (m.size() == 6);

    if (!ok)
    {
      const std::string message = "Failed to parse: " + s;
      throw std::runtime_error(message);
    }

    const std::string note1 = m[1].str();
    const std::string alt1 = m[2].str();
    const int pos1 = getRootNote(note1, alt1, root);

    const std::string mode = m[3].str();
    getAllNotes(pos1, mode, notes);

    const std::string note2 = m[4].str();
    if (!note2.empty())
    {
      const std::string alt2 = m[5].str();
      int pos2 = getRootNote(note2, alt2, root) + 12;

      while (pos2 >= pos1)
      {
	pos2 -= 12;
      }

      notes.insert(notes.begin(), pos2);
    }
  }

  void populateChords(const std::string & filename, std::vector<ASI::Chords::ChordPlayerHandler::ChordData> & chords)
  {
    std::ifstream in(filename);

    chords.clear();

    {
      ASI::Chords::ChordPlayerHandler::ChordData data;
      data.skip = false;
      data.trigger = -1;

      // this will never be checked directly
      // but avoids "if" statements later
      chords.push_back(data);
    }

    std::string line;
    while(std::getline(in, line))
    {
      if (line.empty() || line[0] == '#')
      {
	continue; // skip empty lines and comments
      }

      std::vector<std::string> tokens;
      boost::split(tokens, line, boost::is_any_of(","));

      if (tokens.empty())
      {
	const std::string message = "Failed to parse: " + line;
	throw std::runtime_error(message);
      }

      ASI::Chords::ChordPlayerHandler::ChordData data;
      data.skip = false;
      data.trigger = getNoteAndOctave(tokens[0]);

      if (tokens.size() == 2)
      {
	const std::string & chord = tokens[1];

	if (chord == "SKIP")
	{
	  data.skip = true;
	}
	else if (!chord.empty())
	{
	  createChord(chord, data.notes);
	}
      }

      chords.push_back(data);
    }
  }

  void printChords(const std::vector<ASI::Chords::ChordPlayerHandler::ChordData> & chords)
  {
    for (const auto & chord : chords)
    {
      std::cout << (int)chord.trigger << ",";
      if (chord.skip)
      {
	std::cout << " SKIP";
      }
      else
      {
	for (auto n : chord.notes)
	{
	  std::cout << " " << (int)n;
	}
      }
      std::cout << std::endl;
    }
  }

  void execute(void * buffer, const int velocity, const jack_midi_event_t & event, const ASI::Chords::ChordPlayerHandler::ChordData & data, const jack_midi_data_t cmd)
  {
    const std::vector<jack_midi_data_t> & notes = data.notes;

    const jack_midi_data_t inputVelocity = event.buffer[2];
    const jack_midi_data_t actualVelocity = velocity == 0 ? inputVelocity : velocity & 0x7f;

    for (auto n : notes)
    {
      jack_midi_data_t data[3];
      data[0] = cmd;
      data[1] = n;
      data[2] = actualVelocity;

      jack_midi_event_write(buffer, event.time, data, 3);
    }
  }

}

namespace ASI
{
  namespace Chords
  {

    ChordPlayerHandler::ChordPlayerHandler(jack_client_t * client, PortMapper & mapper, const std::string & filename, const int velocity)
      : InputOutputHandler(client), m_filename(filename), m_velocity(velocity)
    {
      m_inputPort = mapper.registerPort("chord_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
      m_outputPort = mapper.registerPort("chord_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);

      m_previousState = JackTransportStopped;

      populateChords(m_filename, m_chords);

      // this is for debugging only
      if (false) printChords(m_chords);

      m_next = 0;
      reset();
    }

    void ChordPlayerHandler::setNext(const size_t next)
    {
      if (m_next != next)
      {
	m_next = next;

	if (m_next != m_chords.size())
	{
	  const ChordData & next = m_chords[m_next];
	  std::cout << "Waiting for [" << m_next << "]: ";
	  streamNoteName(std::cout, next.trigger, BEST);

	  // write some more " " to clear longer lines followed by shorter ones
	  std::cout << " (" << (int)next.trigger << ")           \r" << std::flush;
	}
	else
	{
	  std::cout << "DONE. Press sostenuto pedal to restart" << std::endl;
	}
      }
    }

    void ChordPlayerHandler::reset()
    {
      m_previous = 0;
      setNext(1);
    }

    void ChordPlayerHandler::process(const jack_nframes_t nframes)
    {
      void* inPortBuf = jack_port_get_buffer(m_inputPort, nframes);
      void* outPortBuf = jack_port_get_buffer(m_outputPort, nframes);

      jack_midi_clear_buffer(outPortBuf);

      const jack_transport_state_t state = jack_transport_query(m_client, nullptr);

      switch (state)
      {
      case JackTransportStopped:
	{
	  if (m_previousState != state)
	  {
	    // make up a reference event
	    // populating only what is needed by execute()
	    jack_midi_event_t inEvent;
	    jack_midi_data_t data[3];
	    inEvent.buffer = data;
	    inEvent.size = sizeof(data);
	    inEvent.buffer[2] = 80; // velocity
	    inEvent.time = 0; // immediately
	    // cancel the last chord played
	    execute(outPortBuf, m_velocity, inEvent, m_chords[m_previous], MIDI_NOTEOFF);
	    // reset pointers
	    reset();
	  }
	  break;
	}
      case JackTransportRolling:
	{
	  jack_nframes_t eventCount = jack_midi_get_event_count(inPortBuf);

	  for (size_t i = 0; i < eventCount; ++i)
	  {
	    jack_midi_event_t inEvent;
	    jack_midi_event_get(&inEvent, inPortBuf, i);

	    if (m_next == m_chords.size())
	    {
	      break;
	    }

	    const jack_midi_data_t cmd = inEvent.buffer[0] & 0xf0;

	    if (cmd == MIDI_NOTEON)
	    {
	      const jack_midi_data_t velocity = inEvent.buffer[2];

	      // MuseScore sends velocity = 0 rather than NOTEOFF
	      if (velocity > 0)
	      {
		const jack_midi_data_t note = inEvent.buffer[1];

		const ChordData & next = m_chords[m_next];
		if (note == next.trigger)
		{
		  if (!next.skip)
		  {
		    execute(outPortBuf, m_velocity, inEvent, m_chords[m_previous], MIDI_NOTEOFF);
		    execute(outPortBuf, m_velocity, inEvent, m_chords[m_next], MIDI_NOTEON);
		    m_previous = m_next;
		  }
		  setNext(m_next + 1);
		}
	      }
	    }
	  }
	  break;
	}
      default:
	{
	  break;
	}
      }

      m_previousState = state;
    }

    void ChordPlayerHandler::shutdown()
    {
    }

  }
}
