#pragma once

#include "SynthParameters.h"
#include <cstddef>

namespace ASI
{
  class InitFilter;

  void createButterBandPassFilter(const size_t order, const size_t sr, const Real_t lower, const Real_t upper, InitFilter & filter);
}
