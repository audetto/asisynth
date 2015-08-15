#include "EchoHandler.h"
#include "ModeHandler.h"
#include "SuperLegatoHandler.h"
#include "ChordPlayerHandler.h"

#include <boost/program_options.hpp>
#include <iostream>

namespace ASI
{
  namespace po = boost::program_options;

  bool createHandlers(int argc, char ** argv, jack_client_t * client,
		      std::vector<std::shared_ptr<I_JackHandler> > & handlers)
  {
    po::options_description desc("ASISynth");
    desc.add_options()
      ("help,h", "Print this help message");

    po::options_description echoDesc("Echo");
    echoDesc.add_options()
      ("echo,e", "Enable echo effect")
      ("echo:delay,d", po::value<double>()->default_value(0.0), "Delay in seconds")
      ("echo:transposition,t", po::value<int>()->default_value(0), "Transposition in semitones")
      ("echo:velocity,v", po::value<double>()->default_value(1.0), "Velocity ratio");
    desc.add(echoDesc);

    po::options_description modeDesc("Mode change");
    modeDesc.add_options()
      ("mode", "Enable Major <-> Minor transposition")
      ("mode:offset", po::value<int>()->default_value(0), "0 C, 1 B, 2 B flat, ..., 11 D flat")
      ("mode:target", po::value<std::string>(), "Target mode: major / minor")
      ("mode:quirk", po::value<std::string>()->default_value("skip"), "Quirk mode: below / skip / above");
    desc.add(modeDesc);

    po::options_description legatoDesc("Super Legato");
    legatoDesc.add_options()
      ("legato", "Super Legato")
      ("legato:delay", po::value<int>()->default_value(0), "NOTEOFF delay in milliseconds");
    desc.add(legatoDesc);

    po::options_description chordDesc("Chord Player");
    chordDesc.add_options()
      ("chords", "Chord Player")
      ("chords:file", po::value<std::string>(), "Chord filename")
      ("chords:velocity", po::value<int>()->default_value(0), "Chord velocity (0 use trigger note's)");
    desc.add(chordDesc);

    po::variables_map vm;
    try
    {
      po::store(po::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
      {
	std::cout << "ASISynth for MIDI effects" << std::endl << std::endl << desc << std::endl;
	return false;
      }

      if (vm.count("echo"))
      {
	const double lag = vm["echo:delay"].as<double>();
	const int transposition = vm["echo:transposition"].as<int>();
	const double velocity = vm["echo:velocity"].as<double>();
	handlers.push_back(std::make_shared<ASI::EchoHandler>(client, lag, transposition, velocity));
      }

      if (vm.count("mode"))
      {
	const int offset = vm["mode:offset"].as<int>();
	const std::string target = vm["mode:target"].as<std::string>();
	const std::string quirk = vm["mode:quirk"].as<std::string>();
	handlers.push_back(std::make_shared<ASI::ModeHandler>(client, offset, target, quirk));
      }

      if (vm.count("legato"))
      {
	const int delay = vm["legato:delay"].as<int>();
	handlers.push_back(std::make_shared<ASI::SuperLegatoHandler>(client, delay));
      }

      if (vm.count("chords"))
      {
	const std::string filename = vm["chords:file"].as<std::string>();
	const int velocity = vm["chords:velocity"].as<int>();
	handlers.push_back(std::make_shared<ASI::ChordPlayerHandler>(client, filename, velocity));
      }
    }
    catch (po::error& e)
    {
      std::cerr << "ERROR: " << e.what() << std::endl << desc << std::endl;
      return false;
    }

    return true;
  }
}
