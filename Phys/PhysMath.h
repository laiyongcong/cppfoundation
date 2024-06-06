#pragma once
#include "Prerequisites.h"

namespace cppfd {
#define __PI (3.1415926535897932f)
#define KINDA_SMALL_NUMBER (1.e-4f)
#define SMALL_NUMBER (1.e-8f)
#define THRESH_VECTOR_NORMALIZED (0.01f)
#define DELTA (0.00001f)
#define DEG_TO_RAD (__PI/180.0)
#define RAD_TO_DEG (180.0/__PI)

// Aux constants.
#define INV_PI (0.31830988618379f)
#define HALF_PI (1.5707963267948966f)

#ifndef MAX_BYTE
#  define MAX_BYTE ((uint8_t)(0xFF))
#endif

#ifndef MAX_INT
#  define MAX_INT ((int32_t)(0x7FFFFFFF))
#endif

#ifndef MIN_INT
#  define MIN_INT ((int32_t)(0x80000000))
#endif

#ifndef MAX_UINT
#  define MAX_UINT ((uint32_t)(0xFFFFFFFF))
#endif

#ifndef MIN_FLOAT
#   define MIN_FLOAT (1.175494351e-38F) /* min positive value */
#endif

#ifndef MAX_FLOAT
#  define MAX_FLOAT (3.402823466e+38F)
#endif

#ifndef MIN_DOUBLE
#  define MIN_DOUBLE (2.2250738585072014e-308) /* min positive value */
#endif  // !MIN_DOUBLE

#ifndef MAX_DOUBLE
#  define MAX_DOUBLE (1.7976931348623158e+308)
#endif

struct PhysMath {
  static double Atan2(double y, double x);
  static void SinCos(double* ScalarSin, double* ScalarCos, double DegVal);
  template <class T>
  static FORCEINLINE auto RadiansToDegrees(T const& RadVal) -> decltype(RadVal * (180.f / __PI)) {
    return RadVal * (180.f / __PI);
  }
  template <class T>
  static FORCEINLINE auto DegreesToRadians(T const& DegVal) -> decltype(DegVal * (__PI / 180.f)) {
    return DegVal * (__PI / 180.f);
  }
  template <class T>
  static FORCEINLINE T Square(const T A) {
    return A * A;
  }
  template <class T>
  static FORCEINLINE T Clamp(const T X, const T Min, const T Max) {
    return X < Min ? Min : X < Max ? X : Max;
  }
  template <class T, class U>
  static FORCEINLINE T Lerp(const T& A, const T& B, const U& Alpha) {
    return (T)(A + Alpha * (B - A));
  }
  /** Given a heading which may be outside the +/- PI range, 'unwind' it back into that range. */
  static FORCEINLINE double UnwindRadians(double A) {
    while (A > __PI) {
      A -= ((double)__PI * 2.0f);
    }

    while (A < -__PI) {
      A += ((double)__PI * 2.0f);
    }

    return A;
  }

  /** Utility to ensure angle is between +/- 180 degrees by unwinding. */
  static FORCEINLINE double UnwindDegrees(double A) {
    while (A > 180.f) {
      A -= 360.f;
    }

    while (A < -180.f) {
      A += 360.f;
    }

    return A;
  }

  /** Computes absolute value in a generic way */
  template <class T>
  static FORCEINLINE T Abs(const T A) {
    return (A >= (T)0) ? A : -A;
  }

  /** Returns higher value in a generic way */
  template <class T>
  static FORCEINLINE T Max(const T A, const T B) {
    return (A >= B) ? A : B;
  }

  /** Returns lower value in a generic way */
  template <class T>
  static FORCEINLINE T Min(const T A, const T B) {
    return (A <= B) ? A : B;
  }

  static FORCEINLINE bool IsNearlyEqual(double A, double B, double ErrorTolerance = SMALL_NUMBER) { return Abs<double>(A - B) <= ErrorTolerance; }

  static FORCEINLINE double Mod(double X, double Y) {
    if (IsNearlyEqual(Y, 0)) return 0.0;
    const double dQuotient = (int32_t)(X / Y);
    double dIntPortion = Y * dQuotient;
    if (fabs(dIntPortion) > fabs(X)) {
      dIntPortion = X;
    }
    return X - dIntPortion;
  }
};

}