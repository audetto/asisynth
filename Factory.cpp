#include <boost/program_options.hpp>

#include "EchoHandler.h"

#include <iostream>

namespace ASI
{
  namespace po = boost::program_options;

  bool createHandlers(int argc, char ** argv, jack_client_t * client,
		      std::vector<std::shared_ptr<I_JackHandler> > & handlers)
  {
    po::options_description desc("Options");

    desc.add_options()
      ("help", "Print help message")
      ("echo", po::value<double>(), "Delay in seconds");

    po::variables_map vm;
    try
    {
      po::store(po::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
      {
	std::cout << "AsiSynth for MIDI effects" << std::endl << desc << std::endl;
	return false;
      }

      if (vm.count("echo"))
      {
	const double lag = vm["echo"].as<double>();
	handlers.push_back(std::make_shared<ASI::EchoHandler>(client, lag));


      }
    }
    catch (po::error& e)
    {
      std::cerr << "ERROR: " << e.what() << std::endl << desc << std::endl;
      return false;
    }
  }
}
