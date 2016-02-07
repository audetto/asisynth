#include "I_JackHandler.h"
#include "Factory.h"

#include <iostream>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>

#include <jack/jack.h>
#include <jack/midiport.h>

#include <unistd.h>
#include <signal.h>

namespace
{

  typedef std::vector<std::shared_ptr<ASI::I_JackHandler> > Handlers_t;

  struct ClientData
  {
    size_t time;
    double maxLoad; // nedd to "* sr / 1000000.0" later
    jack_nframes_t frames;
    jack_nframes_t sr;

    Handlers_t handlers;
    std::atomic<bool> alive;
  };

  // here so it can be accessed by the signal handler
  // but I'd like to keep it on the stack of the app
  ClientData data;

  int process(jack_nframes_t nframes, void *arg)
  {
    auto t0 = std::chrono::high_resolution_clock::now();
    ClientData & data = *reinterpret_cast<ClientData *>(arg);
    Handlers_t & handlers = data.handlers;

    for (auto & handler : handlers)
    {
      handler->process(nframes);
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    const size_t elapsed = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    data.frames += nframes;
    data.time += elapsed;

    const double load = double(elapsed) / double(nframes);

    data.maxLoad = std::max(data.maxLoad, load);
    return 0;
  }

  int sampleRate(jack_nframes_t nframes, void *arg)
  {
    ClientData & data = *reinterpret_cast<ClientData *>(arg);
    Handlers_t & handlers = data.handlers;

    for (auto & handler : handlers)
    {
      handler->sampleRate(nframes);
    }

    data.sr = nframes;
    return 0;
  }

  void shutdown(void *arg)
  {
    ClientData & data = *reinterpret_cast<ClientData *>(arg);
    Handlers_t & handlers = data.handlers;

    for (auto & handler : handlers)
    {
      handler->shutdown();
    }

    data.alive = false;
  }

  void signalHandler(int i)
  {
    data.alive = false;
  }

  void printStatistics(const ClientData & data)
  {
    const double seconds = data.time / 1000000.0;
    const double jack = double(data.frames) / double(data.sr);
    const double load = seconds / jack;

    const double maxLoad = data.maxLoad * data.sr / 1000000.0;

    std::cout << std::endl;
    std::cout << "Frames: " << data.frames << std::endl;
    std::cout << "Seconds: " << seconds << std::endl;
    std::cout << "Jack: " << jack << std::endl;
    std::cout << "Load: " << load * 100.0 << " %" << std::endl;
    std::cout << "Max load: " << maxLoad * 100.0 << " %" << std::endl;
  }

}

int main(int argc, char **args)
{

  jack_client_t * client = jack_client_open("AsiSynth", JackNullOption, nullptr);

  if (!client)
  {
    std::cerr << "jack server not running?" << std::endl;
    return 1;
  }

  std::vector<std::shared_ptr<ASI::I_JackHandler> > & handlers = data.handlers;
  data.alive = false;
  data.time = 0.0;
  data.frames = 0;
  data.sr = 0;

  if (!ASI::createHandlers(argc, args, client, handlers))
  {
    // -h was selected
    return 3;
  }

  if (handlers.empty())
  {
    // nothing to do
    return 0;
  }

  jack_set_process_callback(client, process, &data);

  jack_set_sample_rate_callback(client, sampleRate, &data);

  jack_on_shutdown(client, shutdown, &data);

  signal(SIGINT, &signalHandler);
  signal(SIGTERM, &signalHandler);

  if (jack_activate(client))
  {
    std::cerr << "Cannot activate client" << std::endl;
    return 2;
  }
  data.alive = true;

  /* run until interrupted */
  while(data.alive)
  {
    sleep(1);
  }

  // detach all ports
  jack_client_close(client);

  printStatistics(data);

  return 0;
}
