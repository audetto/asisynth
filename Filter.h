#pragma once

#include "SynthParameters.h"

#include <vector>
#include <array>
#include <cstring>

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

      m_buffer.resize(8192);
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
      for (ssize_t i = 1; i < m_sizeOfAB; ++i)
      {
	m_a[i] /= m_a[0];
      }
      m_a[0] = 0.0;
    }

    // very old
    template <ssize_t sizeOfAB>
    void process_0(Real_t * x, const ssize_t n)
    {
      for (ssize_t j = 0; j < n; ++j)
      {
	// add it
	m_x[m_pos & ((1 << N) - 1)] = x[j];
	// now (till end of function m_pos is the last position written to)

	Real_t sum_y = 0.0;
	Real_t sum_x = 0.0;

	for (ssize_t i = 0; i < sizeOfAB; ++i)
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

	x[j] = y;

	// advance pointers
	++m_pos;
      }
    }

    // old
    template <ssize_t sizeOfAB>
    void process_1(Real_t * x, const ssize_t n)
    {
      // calculation of x
      for (ssize_t j = 0; j < sizeOfAB; ++j)
      {
	Real_t dot = 0.0;
	for (ssize_t i = 0; i < sizeOfAB; ++i)
	{
	  if (j - i >= 0)
	    dot += x[j - i] * m_b[i];
	  else
	    dot += m_x[i - j] * m_b[i];
	}
	m_buffer[j] = dot;
      }

      for (ssize_t j = sizeOfAB; j < n; ++j)
      {
	Real_t dot = 0.0;
	for (ssize_t i = 0; i < sizeOfAB; ++i)
	{
	  dot += x[j - i] * m_b[i];
	}
	m_buffer[j] = dot;
      }

      // prepare for next time
      for (ssize_t i = 1; i < m_sizeOfAB; ++i)
      {
	m_x[i] = x[n - i];
      }

      // calculation of y
      // x is the output
      for (ssize_t j = 0; j < sizeOfAB; ++j)
      {
	Real_t dot = 0.0;
	for (ssize_t i = 1; i < sizeOfAB; ++i)
	{
	  if (j - i >= 0)
	    dot += x[j - i] * m_a[i];
	  else
	    dot += m_y[i - j] * m_a[i];
	}
	x[j] = m_buffer[j] - dot;
      }

      for (ssize_t j = sizeOfAB; j < n; ++j)
      {
	Real_t dot = 0.0;
	for (ssize_t i = 1; i < sizeOfAB; ++i)
	{
	  dot += x[j - i] * m_a[i];
	}
	x[j] = m_buffer[j] - dot;
      }

      // prepare for next time
      for (ssize_t i = 1; i < sizeOfAB; ++i)
      {
	m_y[i] = x[n - i];
      }
    }

    // new
    template <ssize_t sizeOfAB>
    void process_2(Real_t * x, const ssize_t n)
    {
      memset(m_buffer.data(), 0, sizeof(Real_t) * n);

      // calculation of x
      for (ssize_t i = 0; i < sizeOfAB; ++i)
      {
	for (ssize_t j = 0; j < i; ++j)
	{
	  // i - j >= 1
	  m_buffer[j] += m_x[i - j] * m_b[i];
	}
	for (ssize_t j = i; j < n; ++j)
	{
	  // j - i >= 0
	  m_buffer[j] += x[j - i] * m_b[i];
	}
      }

      // prepare for next time
      for (ssize_t i = 1; i < sizeOfAB; ++i)
      {
	m_x[i] = x[n - i];
      }

      // calculation of y
      // x is the output
      for (ssize_t j = 0; j < sizeOfAB; ++j)
      {
	Real_t dot = 0.0;
	for (ssize_t i = 1; i <= j; ++i)
	{
	  dot += x[j - i] * m_a[i];
	}
	for (ssize_t i = j + 1; i < sizeOfAB; ++i)
	{
	  dot += m_y[i - j] * m_a[i];
	}
	x[j] = m_buffer[j] - dot;
      }

      for (ssize_t j = sizeOfAB; j < n; ++j)
      {
	Real_t dot = 0.0;
	for (ssize_t i = 1; i < sizeOfAB; ++i)
	{
	  dot += x[j - i] * m_a[i];
	}
	x[j] = m_buffer[j] - dot;
      }

      // prepare for next time
      for (ssize_t i = 1; i < sizeOfAB; ++i)
      {
	m_y[i] = x[n - i];
      }
    }

    void process(Real_t * x, const ssize_t n)
    {
      switch (m_sizeOfAB)
      {
      case 1:
	return; // this is just y_i = x_i, so we skip it.
      case 3:
	return process_2<3>(x, n);
      case 5:
	return process_2<5>(x, n);
      };
    }

  private:

    ssize_t m_pos;
    std::array<Real_t, 1 << N> m_x;
    std::array<Real_t, 1 << N> m_y;

    ssize_t m_sizeOfAB;
    std::array<Real_t, 1 << N> m_b;
    std::array<Real_t, 1 << N> m_a;

    std::vector<Real_t> m_buffer;
  };

}
