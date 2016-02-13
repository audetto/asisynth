#pragma once

#include <cstddef>

namespace ASI
{
  class InitFilter;

  void createButterBandPassFilter(const int order, const size_t sr, const double lower, const double upper, InitFilter & filter);
}
