#include "IIRFactory.h"
#include "Filter.h"

#include <cstdlib>

extern "C"
{
#include "sigproc/iir.h"
}

namespace ASI
{
  void createButterBandPassFilter(const size_t order, const size_t sr, const double lower, const double upper, InitFilter & filter)
  {
    const double wl = 2.0 * lower / sr;
    const double wh = 2.0 * upper / sr;

    int * ccof = ccof_bwbp(order);             // b
    double * dcof = dcof_bwbp(order, wl, wh);  // a
    if (!ccof || !dcof)
    {
      throw std::runtime_error("Cannot create filter");
    }

    const double scalingFactor = sf_bwbp(order, wl, wh);

    const size_t numberOfCoefficients = 2 * order + 1;

    std::vector<double> b(ccof, ccof + numberOfCoefficients);
    for (double & value: b)
    {
      value *= scalingFactor;
    }

    std::vector<double> a(dcof, dcof + numberOfCoefficients);

    filter.init(b, a);

    // we are probably leaking if an exception is thrown above
    free(ccof);
    free(dcof);
  }

}
