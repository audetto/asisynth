#pragma once

#include "SynthParameters.h"

#include <vector>
#include <array>

namespace ASI
{
  class InitFilter
  {
  public:
    virtual void init(std::vector<Real_t> b, std::vector<Real_t> a) = 0;
  };

  template <ssize_t N>
    class Filter : public InitFilter
  {
  public:
    Filter()
    {
      // this is the next position to write to
      m_pos = 0;

      m_x.fill(0.0);
      m_y.fill(0.0);
      m_b.fill(0.0);
      m_a.fill(0.0);

      m_b[0] = 1.0;
      m_a[0] = 0.0;               // a is normalised!
      m_sizeOfAB = 1;
    }

    virtual void init(std::vector<Real_t> b, std::vector<Real_t> a) override
    {
      if (b.size() > m_b.size())
      {
	throw std::invalid_argument("IIR a is too large");
      }

      if (a.size() > m_a.size())
      {
	throw std::invalid_argument("IIR a is too large");
      }

      if (a.empty())
      {
	throw std::invalid_argument("IIR a is empty");
      }

      m_b.fill(0.0);
      m_a.fill(0.0);
      std::copy(b.begin(), b.end(), m_b.begin());
      std::copy(a.begin(), a.end(), m_a.begin());

      m_sizeOfAB = std::max(b.size(), a.size());

      // normalise so we do not need to worry about m_a[0] later
      for (size_t i = 1; i < m_sizeOfAB; ++i)
      {
	m_a[i] /= m_a[0];
      }
      m_a[0] = 0.0;
    }

    void process(const Real_t * x, const size_t n)
    {
      for (size_t i = 0; i < n; ++i)
      {
	// add it
	m_x[m_pos & ((1 << N) - 1)] = x[i];
	// now (till end of function m_pos is the last position written to)

	Real_t sum_y = 0.0;
	Real_t sum_x = 0.0;
	for (size_t i = 0; i < m_sizeOfAB; ++i)
	{
	  // on the first iteration we read from m_pos
	  // which has just been written to
	  const ssize_t k = (m_pos - i) & ((1 << N) - 1);
	  sum_x += m_x[k] * m_b[i];

	  // on the first iteration m_a[0] = 0
	  // the,the next we read from m_pos - 1
	  // which was written to on the previous call to ::process()
	  sum_y += m_y[k] * m_a[i];
	}

	const Real_t y = sum_x - sum_y;

	// write to
	m_y[m_pos & ((1 << N) - 1)] = y;

	// advance pointers
	++m_pos;
      }
    }

  private:

    ssize_t m_pos;
    std::array<Real_t, 1 << N> m_x;
    std::array<Real_t, 1 << N> m_y;

    size_t m_sizeOfAB;
    std::array<Real_t, 1 << N> m_b;
    std::array<Real_t, 1 << N> m_a;
  };

}
