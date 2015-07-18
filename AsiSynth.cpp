#include "I_JackHandler.h"
#include "Factory.h"

#include <iostream>
#include <vector>
#include <memory>
#include <atomic>

#include <jack/jack.h>
#include <jack/midiport.h>

#include <unistd.h>

namespace
{

  typedef std::vector<std::shared_ptr<ASI::I_JackHandler> > Handlers_t;

  struct ClientData
  {
    Handlers_t handlers;
    std::atomic<bool> alive;
  };

  int process(jack_nframes_t nframes, void *arg)
  {
    ClientData & data = *reinterpret_cast<ClientData *>(arg);
    Handlers_t & handlers = data.handlers;

    for (auto & handler : handlers)
    {
      int res = handler->process(nframes);
      if (res)
	return res;
    }
    return 0;
  }

  int sampleRate(jack_nframes_t nframes, void *arg)
  {
    ClientData & data = *reinterpret_cast<ClientData *>(arg);
    Handlers_t & handlers = data.handlers;

    for (auto & handler : handlers)
    {
      int res = handler->sampleRate(nframes);
      if (res)
	return res;
    }
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

}

int main(int argc, char **args)
{

  jack_client_t * client = jack_client_open("AsiSynth", JackNullOption, NULL);

  if (!client)
  {
    std::cerr << "jack server not running?" << std::endl;
    return 1;
  }

  ClientData data;
  std::vector<std::shared_ptr<ASI::I_JackHandler> > & handlers = data.handlers;
  data.alive = false;

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

  jack_set_process_callback(client, process, &handlers);

  jack_set_sample_rate_callback(client, sampleRate, &handlers);

  jack_on_shutdown(client, shutdown, &handlers);

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

  handlers.clear();
  jack_client_close(client);
  return 0;
}
