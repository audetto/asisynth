#pragma once

#include <vector>
#include <array>

namespace ASI
{
  class InitFilter
  {
  public:
    virtual void init(std::vector<double> b, std::vector<double> a) = 0;
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
      m_a[0] = 1.0;
      m_sizeOfB = 1;
      m_sizeOfA = 1;
    }

    virtual void init(std::vector<double> b, std::vector<double> a) override
    {
      if (b.size() > m_b.size())
      {
	throw std::invalid_argument("IIR a is too large");
      }

      if (a.size() > m_a.size())
      {
	throw std::invalid_argument("IIR a is too large");
      }

      std::copy(b.begin(), b.end(), m_b.begin());
      std::copy(a.begin(), a.end(), m_a.begin());

      m_sizeOfB = b.size();
      m_sizeOfA = a.size();

      // normalise so we do not need to worry about m_a[0] later
      for (size_t i = 1; i < m_sizeOfA; ++i)
      {
	m_a[i] /= m_a[0];
      }
    }

    double process(const double x)
    {
      // add it
      m_x[m_pos & ((1 << N) - 1)] = x;
      // now (till end of function m_posX is the last position written to)

      double y = 0;

      for (ssize_t i = 0; i < m_sizeOfB; ++i)
      {
	// on the first iteration we read from m_posX
	// which has just been written to
	const ssize_t k = (m_pos - i) & ((1 << N) - 1);
	y += m_x[k] * m_b[i];
      }

      for (ssize_t i = 1; i < m_sizeOfA; ++i)
      {
	// on the first iteration we read from m_posY - 1
	// which was written to on the previous call to ::process()
	const ssize_t k = (m_pos - i) & ((1 << N) - 1);
	y -= m_y[k] * m_a[i];
      }

      // write to
      m_y[m_pos & ((1 << N) - 1)] = y;

      // advance pointers
      ++m_pos;

      return y;
    }

  private:

    ssize_t m_pos;
    std::array<double, 1 << N> m_x;
    std::array<double, 1 << N> m_y;

    size_t m_sizeOfB;
    size_t m_sizeOfA;
    std::array<double, 1 << N> m_b;
    std::array<double, 1 << N> m_a;
  };

}
