#include "PhysMath.h"

namespace cppfd {

double PhysMath::Atan2(double y, double x) {
  const double absX = fabs(x);
  const double absY = fabs(y);
  const bool yAbsBigger = (absY > absX);
  double t0 = yAbsBigger ? absY : absX;  // Max(absY, absX)
  double t1 = yAbsBigger ? absX : absY;  // Min(absX, absY)

  if (t0 == 0.f) return 0.f;

  double t3 = t1 / t0;
  double t4 = t3 * t3;

  static const double c[7] = {+7.2128853633444123e-03f, -3.5059680836411644e-02f, +8.1675882859940430e-02f, -1.3374657325451267e-01f, +1.9856563505717162e-01f, -3.3324998579202170e-01f, +1.0f};

  t0 = c[0];
  t0 = t0 * t4 + c[1];
  t0 = t0 * t4 + c[2];
  t0 = t0 * t4 + c[3];
  t0 = t0 * t4 + c[4];
  t0 = t0 * t4 + c[5];
  t0 = t0 * t4 + c[6];
  t3 = t0 * t3;

  t3 = yAbsBigger ? (0.5f * __PI) - t3 : t3;
  t3 = (x < 0.0f) ? __PI - t3 : t3;
  t3 = (y < 0.0f) ? -t3 : t3;

  return t3;
}

void PhysMath::SinCos(double* ScalarSin, double* ScalarCos, double dRad) {
  double quotient = (INV_PI * 0.5f) * dRad;
  if (dRad >= 0.0f) {
    quotient = (double)((int64_t)(quotient + 0.5f));
  } else {
    quotient = (double)((int64_t)(quotient - 0.5f));
  }
  double y = dRad - (2.0f * __PI) * quotient;

  // Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
  double sign;
  if (y > HALF_PI) {
    y = __PI - y;
    sign = -1.0f;
  } else if (y < -HALF_PI) {
    y = -__PI - y;
    sign = -1.0f;
  } else {
    sign = +1.0f;
  }

  double y2 = y * y;

  // 11-degree minimax approximation
  *ScalarSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

  // 10-degree minimax approximation
  double p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
  *ScalarCos = sign * p;
}

}