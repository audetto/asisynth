#pragma once

#include "handlers/synth/SynthParameters.h"
#include <cstddef>

namespace ASI
{
  namespace Synth
  {
    class InitFilter;

    void createButterBandPassFilter(const size_t order, const size_t sr, const Real_t lower, const Real_t upper, InitFilter & filter);
    void createFilter(const Pass pass, const size_t order, const size_t sr, const Real_t lower, const Real_t upper, InitFilter & filter);
  }
}
