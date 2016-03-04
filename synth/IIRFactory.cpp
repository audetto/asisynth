#include "IIRFactory.h"
#include "Filter.h"

#include <cstdlib>

extern "C"
{
#include "../sigproc/iir.h"
}

namespace ASI
{
  namespace Synth
  {

    void createFilter(const Pass pass, const size_t order, const size_t sr, const Real_t lower, const Real_t upper, InitFilter & filter)
    {
      switch (pass)
      {
      case Pass::BANDPASS:
	{
	  createButterBandPassFilter(order, sr, lower, upper, filter);
	  break;
	}
      case Pass::NONE:
      default:
	{
	  filter.resetFilter();
	  break;
	}
      }
    }


    void createButterBandPassFilter(const size_t order, const size_t sr, const Real_t lower, const Real_t upper, InitFilter & filter)
    {
      const Real_t wl = 2.0 * lower / sr;
      const Real_t wh = 2.0 * upper / sr;

      int * ccof = ccof_bwbp(order);             // b
      double * dcof = dcof_bwbp(order, wl, wh);  // a
      if (!ccof || !dcof)
      {
	throw std::runtime_error("Cannot create filter");
      }

      const Real_t scalingFactor = sf_bwbp(order, wl, wh);

      const size_t numberOfCoefficients = 2 * order + 1;

      std::vector<Real_t> b(ccof, ccof + numberOfCoefficients);
      for (Real_t & value: b)
      {
	value *= scalingFactor;
      }

      std::vector<Real_t> a(dcof, dcof + numberOfCoefficients);

      filter.init(b, a);

      // we are probably leaking if an exception is thrown above
      free(ccof);
      free(dcof);
    }

  }
}
