#pragma once

#include <cstddef>

namespace ASI
{
  class InitFilter;

  void createButterBandPassFilter(const size_t order, const size_t sr, const double lower, const double upper, InitFilter & filter);
}
