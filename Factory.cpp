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
      ("help,h", "Print this help message");

    po::options_description echoDesc("Echo");
    echoDesc.add_options()
      ("echo,e", "Enable echo effect")
      ("echo:delay,d", po::value<double>()->default_value(0.0), "Delay in seconds")
      ("echo:transposition,t", po::value<int>()->default_value(0), "Transposition in semitones")
      ("echo:velocity,v", po::value<double>()->default_value(1.0), "Velocity ratio");

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
	const int transposition = vm["echo:transposition"].as<int>();
	const double velocity = vm["echo:velocity"].as<double>();
	handlers.push_back(std::make_shared<ASI::EchoHandler>(client, lag, transposition, velocity));
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
