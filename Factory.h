#include "I_JackHandler.h"

#include <vector>
#include <memory>

#include <jack/jack.h>

namespace ASI
{
  bool createHandlers(int argc, char ** argv, jack_client_t * client,
		      std::vector<std::shared_ptr<I_JackHandler> > & handlers);
}
