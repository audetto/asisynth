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
      : m_b({1.0}), m_a({1.0})
    {
      // this is the next position to write to
      m_posX = 0;
      m_posY = 0;

      std::fill(m_x.begin(), m_x.end(), 0.0);
      std::fill(m_y.begin(), m_y.end(), 0.0);
    }

    virtual void init(std::vector<double> b, std::vector<double> a) override
    {
      m_b = b;
      m_a = a;
    }

    double process(const double x)
    {
      const ssize_t sizeOfB = m_b.size();
      const ssize_t sizeOfA = m_a.size();

      // add it
      m_x[m_posX & ((1 << N) - 1)] = x;
      // now (till end of function m_posX is the last position written to)

      double y = 0;

      for (ssize_t i = 0; i < sizeOfB; ++i)
      {
	// on the first iteration we read from m_posX
	// which has just been written to
	const ssize_t k = (m_posX - i) & ((1 << N) - 1);
	y += m_x[k] * m_b[i];
      }

      for (ssize_t i = 1; i < sizeOfA; ++i)
      {
	// on the first iteration we read from m_posY - 1
	// which was written to on the previous call to ::process()
	const ssize_t k = (m_posY - i) & ((1 << N) - 1);
	y -= m_y[k] * m_a[i];
      }

      y /= m_a[0];

      // write to
      m_y[m_posY & ((1 << N) - 1)] = y;

      // advance pointers
      ++m_posY;
      ++m_posX;

      return y;
    }

  private:

    ssize_t m_posX;
    std::array<double, 1 << N> m_x;

    ssize_t m_posY;
    std::array<double, 1 << N> m_y;

    std::vector<double> m_b;
    std::vector<double> m_a;
  };

}
