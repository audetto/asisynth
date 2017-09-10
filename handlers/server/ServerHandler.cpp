#include "handlers/server/ServerHandler.h"
#include "MidiCommands.h"
#include "MidiUtils.h"
#include "CommonControls.h"

#include <zmq.h>
#include <cassert>
#include <cstring>

#define JACK_RINGBUFFER_SIZE 16384

namespace
{

  void throw_errno(const bool ok)
  {
    if (!ok)
    {
      const char * str = strerror(errno);
      throw std::runtime_error(str);
    }
  }

  void server_thread(ASI::Server::ServerHandler * server)
  {
    void * socket = server->getSocket();

    while (true)
    {
      zmq_msg_t message;
      zmq_msg_init(&message);
      int rc = zmq_msg_recv(&message, socket, 0);
      if (rc == -1)
      {
	break;
      }
      const size_t size = zmq_msg_size(&message);
      const void * data = zmq_msg_data(&message);

      server->write(data, size);

      zmq_msg_close(&message);
    }
  }

}

namespace ASI
{

  namespace Server
  {

    ServerHandler::ServerHandler(const std::shared_ptr<CommonControls> & common, const std::string & endpoint)
      : m_common(common)
    {
      m_outputPort = m_common->registerPort("server_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);

      // this idea comes from RtMidi
      m_buffSize.reset(jack_ringbuffer_create(JACK_RINGBUFFER_SIZE), jack_ringbuffer_free);
      m_buffMessage.reset(jack_ringbuffer_create(JACK_RINGBUFFER_SIZE), jack_ringbuffer_free);

      m_context.reset(zmq_ctx_new(), zmq_ctx_destroy);
      throw_errno(bool(m_context));
      m_socket.reset(zmq_socket(m_context.get(), ZMQ_SUB), zmq_close);
      throw_errno(bool(m_socket));

      int rc = zmq_connect(m_socket.get(), endpoint.c_str());
      throw_errno(rc == 0);

      rc = zmq_setsockopt(m_socket.get(), ZMQ_SUBSCRIBE, nullptr, 0);
      throw_errno(rc == 0);

      m_serverThread = std::thread(server_thread, this);
    }

    void ServerHandler::process(const jack_nframes_t nframes)
    {
      void * outPortBuf = jack_port_get_buffer(m_outputPort, nframes);
      jack_midi_clear_buffer(outPortBuf);

      jack_ringbuffer_t * buffSize = m_buffSize.get();

      while (jack_ringbuffer_read_space(buffSize) > 0)
      {
	size_t space;
	jack_ringbuffer_read(buffSize, (char *)&space, sizeof(space));
	jack_midi_data_t * midiData = jack_midi_event_reserve(outPortBuf, 0, space);

	jack_ringbuffer_read(m_buffMessage.get(), (char *)midiData, space);
      }
    }

    void * ServerHandler::getSocket()
    {
      return m_socket.get();
    }

    void ServerHandler::shutdown()
    {
    }

    ServerHandler::~ServerHandler()
    {
      m_socket.reset();
      m_context.reset();
      try
      {
	m_serverThread.join();
      }
      catch(const std::exception &)
      {
      }
    }

    void ServerHandler::write(const void * message, const size_t size)
    {
      jack_ringbuffer_write(m_buffMessage.get(), (const char *)message, size);
      // make sure we write the size only after the data
      // as as soon as the size is read, the data is read as well
      jack_ringbuffer_write(m_buffSize.get(), (const char *)&size, sizeof(size));
    }

  }
}
