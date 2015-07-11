#include "I_JackHandler.h"
#include "EchoHandler.h"

#include <iostream>
#include <vector>
#include <memory>

#include <jack/jack.h>
#include <jack/midiport.h>

#include <unistd.h>

typedef std::vector<std::shared_ptr<ASI::I_JackHandler> > Handlers_t;

namespace
{

  int process(jack_nframes_t nframes, void *arg)
  {
    Handlers_t & handlers = *reinterpret_cast<Handlers_t *>(arg);

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
    Handlers_t & handlers = *reinterpret_cast<Handlers_t *>(arg);

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
    Handlers_t & handlers = *reinterpret_cast<Handlers_t *>(arg);

    for (auto & handler : handlers)
    {
      handler->shutdown();
    }
  }

}

int main(int narg, char **args)
{

  jack_client_t * client = jack_client_open ("AsiSynth", JackNullOption, NULL);

  if (!client)
  {
    std::cerr << "jack server not running?" << std::endl;
    return 1;
  }

  std::vector<std::shared_ptr<ASI::I_JackHandler> > handlers;

  handlers.push_back(std::make_shared<ASI::EchoHandler>(client, 1.0));

  jack_set_process_callback(client, process, &handlers);

  jack_set_sample_rate_callback(client, sampleRate, &handlers);

  jack_on_shutdown(client, shutdown, &handlers);

  if (jack_activate(client))
  {
    std::cerr << "Cannot activate client" << std::endl;
    return 2;
  }

  /* run until interrupted */
  while(1)
  {
    sleep(1);
  }

  jack_client_close(client);
  return 0;
}
