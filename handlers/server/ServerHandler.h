#pragma once

#include "I_JackHandler.h"

#include <jack/ringbuffer.h>
#include <jack/midiport.h>
#include <string>
#include <vector>
#include <memory>
#include <thread>

namespace ASI
{

  class CommonControls;

  namespace Server
  {

    class ServerHandler : public I_JackHandler
    {
    public:

      ServerHandler(const std::shared_ptr<CommonControls> & common, const std::string & endpoint);
      ~ServerHandler();

      virtual void process(const jack_nframes_t nframes);

      virtual void shutdown();

      void write(const void * message, const size_t size);

     private:

      const std::shared_ptr<CommonControls> m_common;

      jack_port_t *m_outputPort;

      std::shared_ptr<jack_ringbuffer_t> m_buffSize;
      std::shared_ptr<jack_ringbuffer_t> m_buffMessage;

      std::shared_ptr<void> m_context;

      std::thread m_serverThread;


    };

  }
}
