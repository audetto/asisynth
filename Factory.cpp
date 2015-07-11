#include "EchoHandler.h"

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
      ("help", "Print this help message");

    po::options_description echoDesc("Echo");
    echoDesc.add_options()
      ("echo", "Enable echo effect")
      ("echo:delay", po::value<double>()->default_value(0.0), "Delay in seconds")
      ("echo:offset", po::value<int>()->default_value(0), "Note offset in semitones");

    desc.add(echoDesc);

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
	const int offset = vm["echo:offset"].as<int>();
	handlers.push_back(std::make_shared<ASI::EchoHandler>(client, lag, offset));
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
