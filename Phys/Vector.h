#pragma once
#include "Prerequisites.h"
#include "PhysMath.h"

namespace cppfd {
struct Vector3D {  // 左手坐标系 , 参考虚幻的代码
  double mX;       // 正方向，向前
  double mY;       // 正方向，向右
  double mZ;       // 正方向，向上
  explicit FORCEINLINE Vector3D() : mX(0.f), mY(0.f), mZ(0.f) {}
  explicit FORCEINLINE Vector3D(double inF) : mX(inF), mY(inF), mZ(inF) {}
  explicit FORCEINLINE Vector3D(double fx, double fy, double fz) : mX(fx), mY(fy), mZ(fz) {}
  FORCEINLINE Vector3D(const Vector3D& V) : mX(V.mX), mY(V.mY), mZ(V.mZ) {}

  FORCEINLINE Vector3D& operator=(const Vector3D& oth) {
    mX = oth.mX;
    mY = oth.mY;
    mZ = oth.mZ;
    return *this;
  }

  FORCEINLINE bool Equals(const Vector3D& oth, double dTolerance = SMALL_NUMBER) const {
    return PhysMath::Abs(mX - oth.mX) <= dTolerance && PhysMath::Abs(mY - oth.mY) <= dTolerance && PhysMath::Abs(mZ - oth.mZ) <= dTolerance;
  }
  FORCEINLINE bool Equals2D(const Vector3D& oth, double dTolerance = SMALL_NUMBER) const { return PhysMath::Abs(mX - oth.mX) <= dTolerance && PhysMath::Abs(mY - oth.mY) <= dTolerance; }
  FORCEINLINE Vector3D operator^(const Vector3D& oth) const { return Vector3D(mY * oth.mZ - mZ * oth.mY, mZ * oth.mX - mX * oth.mZ, mX * oth.mY - mY * oth.mX); }  // Cross
  FORCEINLINE double operator|(const Vector3D& oth) const { return mX * oth.mX + mY * oth.mY + mZ * oth.mZ; }                                                      // dot

  FORCEINLINE Vector3D operator+(const Vector3D& oth) const { return Vector3D(mX + oth.mX, mY + oth.mY, mZ + oth.mZ); }
  FORCEINLINE Vector3D operator-(const Vector3D& oth) const { return Vector3D(mX - oth.mX, mY - oth.mY, mZ - oth.mZ); }
  FORCEINLINE Vector3D operator*(const Vector3D& oth) const { return Vector3D(mX * oth.mX, mY * oth.mY, mZ * oth.mZ); }
  FORCEINLINE Vector3D operator/(const Vector3D& oth) const { return Vector3D(mX / oth.mX, mY / oth.mY, mZ / oth.mZ); }
  FORCEINLINE Vector3D operator+(double dVal) const { return Vector3D(mX + dVal, mY + dVal, mZ + dVal); }
  FORCEINLINE Vector3D operator-(double dVal) const { return Vector3D(mX - dVal, mY - dVal, mZ - dVal); }
  FORCEINLINE Vector3D operator*(double dVal) const { return Vector3D(mX * dVal, mY * dVal, mZ * dVal); }
  FORCEINLINE Vector3D operator/(double dVal) const { return Vector3D(mX / dVal, mY / dVal, mZ / dVal); }
  FORCEINLINE Vector3D operator+=(const Vector3D& oth) {
    mX += oth.mX;
    mY += oth.mY;
    mZ += oth.mZ;
    return *this;
  }
  FORCEINLINE Vector3D operator-=(const Vector3D& oth) {
    mX -= oth.mX;
    mY -= oth.mY;
    mZ -= oth.mZ;
    return *this;
  }
  FORCEINLINE Vector3D operator*=(const Vector3D& oth) {
    mX *= oth.mX;
    mY *= oth.mY;
    mZ *= oth.mZ;
    return *this;
  }
  FORCEINLINE Vector3D operator/=(const Vector3D& oth) {
    mX /= oth.mX;
    mY /= oth.mY;
    mZ /= oth.mZ;
    return *this;
  }

  FORCEINLINE Vector3D operator+=(double dVal) {
    mX += dVal;
    mY += dVal;
    mZ += dVal;
    return *this;
  }
  FORCEINLINE Vector3D operator-=(double dVal) {
    mX -= dVal;
    mY -= dVal;
    mZ -= dVal;
    return *this;
  }
  FORCEINLINE Vector3D operator*=(double dVal) {
    mX *= dVal;
    mY *= dVal;
    mZ *= dVal;
    return *this;
  }
  FORCEINLINE Vector3D operator/=(double dVal) {
    mX /= dVal;
    mY /= dVal;
    mZ /= dVal;
    return *this;
  }
  // 单位向量上的投影，入参必须是单位向量
  FORCEINLINE Vector3D ProjectOnNormal(const Vector3D& Normal) const { return Normal * (*this | Normal); }
  // 垂直于单位向量的分量，入参必须是单位向量
  FORCEINLINE Vector3D PerpendicularOnNormal(const Vector3D& Normal) const { return *this - Normal * (*this | Normal); }
  // 归一化，转化为单位向量
  FORCEINLINE bool Normalize(double dTolerance = SMALL_NUMBER) {
    const double dSquareSim = mX * mX + mY * mY + mZ * mZ;
    if (dSquareSim > dTolerance) {
      const double scale = 1.0 / ::sqrt(dSquareSim);
      this->operator*=(scale);
      return true;
    }
    return false;
  }
  FORCEINLINE double LengthSquared() const { return mX * mX + mY * mY + mZ * mZ; }
  FORCEINLINE double Length() const { return ::sqrt(mX * mX + mY * mY + mZ * mZ); }
  FORCEINLINE double Distance(const Vector3D& V) const { return ::sqrt((mX - V.mX) * (mX - V.mX) + (mY - V.mY) * (mY - V.mY) + (mZ - V.mZ) * (mZ - V.mZ)); }
  FORCEINLINE double DistanceSquared(const Vector3D& V) const { return ((mX - V.mX) * (mX - V.mX) + (mY - V.mY) * (mY - V.mY) + (mZ - V.mZ) * (mZ - V.mZ)); }

  FORCEINLINE double LengthSquared2D() const { return mX * mX + mY * mY; }
  FORCEINLINE double Length2D() const { return ::sqrt(mX * mX + mY * mY); }
  FORCEINLINE double Distance2D(const Vector3D& V) const { return ::sqrt((mX - V.mX) * (mX - V.mX) + (mY - V.mY) * (mY - V.mY)); }
  FORCEINLINE double DistanceSquared2D(const Vector3D& V) const { return ((mX - V.mX) * (mX - V.mX) + (mY - V.mY) * (mY - V.mY)); }

  FORCEINLINE double CosineAngle(const Vector3D& V) const {
    Vector3D A(*this);
    Vector3D B(V);
    A.Normalize();
    B.Normalize();
    return A | B;
  }
  FORCEINLINE double CosineAngle2D(const Vector3D& V) const {
    Vector3D A(*this);
    Vector3D B(V);
    A.mZ = B.mZ = 0;
    A.Normalize();
    B.Normalize();
    return A | B;
  }
  // 绕单位向量旋转一定弧度, 左手坐标系，顺时针为正方向
  FORCEINLINE Vector3D RotateAroundNormal(const Vector3D& Normal, double dRad) const {
    double dCos = 0, dSin = 0;
    PhysMath::SinCos(&dSin, &dCos, dRad);
    return *this * dCos + (Normal ^ *this) * dSin + Normal * (Normal | *this) * (1 - dCos);
  }
};

struct Quat {
  double X;
  double Y;
  double Z;
  double W;

  explicit FORCEINLINE Quat() : X(0.f), Y(0.f), Z(0.f), W(0.f) {}
  explicit FORCEINLINE Quat(double inF) : X(inF), Y(inF), Z(inF), W(inF) {}
  explicit FORCEINLINE Quat(double fx, double fy, double fz, double fw) : X(fx), Y(fy), Z(fz), W(fw) {}
  FORCEINLINE Quat(const Quat& V) : X(V.X), Y(V.Y), Z(V.Z), W(V.W) {}
  // 从单位向量构造四元数
  FORCEINLINE Quat(const Vector3D& Normal, double dRad) {
    double dS, dC;
    PhysMath::SinCos(&dS, &dC, dRad);
    X = Normal.mX * dS;
    Y = Normal.mY * dS;
    Z = Normal.mZ * dS;
    W = dC;
  }

  FORCEINLINE Quat(const Vector3D V) {
    X = V.mX;
    Y = V.mY;
    Z = V.mZ;
    W = 0;
  }

  // 共轭四元数
  FORCEINLINE Quat Conjugate() const { return Quat(-X, -Y, -Z, W); }
  // 求逆
  FORCEINLINE Quat Inverse() const { return Conjugate() / LengthSquared(); }

  FORCEINLINE double LengthSquared() const { return X * X + Y * Y + Z * Z + W * W; }

  FORCEINLINE double Length() const { return ::sqrt(X * X + Y * Y + Z * Z + W * W); }
  // 叉乘
  FORCEINLINE Quat operator^(const Quat& V) const {
    return Quat(W * V.X + X * V.W + Y * V.Z - Z * V.Y, W * V.Y + Y * V.W + Z * V.X - X * V.Z, W * V.Z + Z * V.W + X * V.Y - Y * V.X, W * V.W - X * V.X - Y * V.Y - Z * V.Z);
  }
  // 点乘
  FORCEINLINE double operator|(const Quat& V) const { return W * V.W + X * V.X + Y * V.Y + Z * V.Z; }

  FORCEINLINE Quat operator+(const Quat& oth) const { return Quat(X + oth.X, Y + oth.Y, Z + oth.Z, W + oth.W); }
  FORCEINLINE Quat operator-(const Quat& oth) const { return Quat(X - oth.X, Y - oth.Y, Z - oth.Z, W - oth.W); }
  FORCEINLINE Quat operator*(const Quat& oth) const { return Quat(X * oth.X, Y * oth.Y, Z * oth.Z, W * oth.W); }
  FORCEINLINE Quat operator/(const Quat& oth) const { return Quat(X / oth.X, Y / oth.Y, Z / oth.Z, W / oth.W); }
  FORCEINLINE Quat operator+(double dVal) const { return Quat(X + dVal, Y + dVal, Z + dVal, W + dVal); }
  FORCEINLINE Quat operator-(double dVal) const { return Quat(X - dVal, Y - dVal, Z - dVal, W - dVal); }
  FORCEINLINE Quat operator*(double dVal) const { return Quat(X * dVal, Y * dVal, Z * dVal, W * dVal); }
  FORCEINLINE Quat operator/(double dVal) const { return Quat(X / dVal, Y / dVal, Z / dVal, W / dVal); }
  FORCEINLINE Quat operator+=(const Quat& oth) {
    X += oth.X;
    Y += oth.Y;
    Z += oth.Z;
    W += oth.W;
    return *this;
  }
  FORCEINLINE Quat operator-=(const Quat& oth) {
    X -= oth.X;
    Y -= oth.Y;
    Z -= oth.Z;
    W -= oth.W;
    return *this;
  }
  FORCEINLINE Quat operator*=(const Quat& oth) {
    X *= oth.X;
    Y *= oth.Y;
    Z *= oth.Z;
    W *= oth.W;
    return *this;
  }
  FORCEINLINE Quat operator/=(const Quat& oth) {
    X /= oth.X;
    Y /= oth.Y;
    Z /= oth.Z;
    W /= oth.W;
    return *this;
  }

  FORCEINLINE Quat operator+=(double dVal) {
    X += dVal;
    Y += dVal;
    Z += dVal;
    W += dVal;
    return *this;
  }
  FORCEINLINE Quat operator-=(double dVal) {
    X -= dVal;
    Y -= dVal;
    Z -= dVal;
    W -= dVal;
    return *this;
  }
  FORCEINLINE Quat operator*=(double dVal) {
    X *= dVal;
    Y *= dVal;
    Z *= dVal;
    W *= dVal;
    return *this;
  }
  FORCEINLINE Quat operator/=(double dVal) {
    X /= dVal;
    Y /= dVal;
    Z /= dVal;
    W /= dVal;
    return *this;
  }

  FORCEINLINE bool Normalize(double dTolerance = SMALL_NUMBER) {
    const double dSquareSim = X * X + Y * Y + Z * Z + W * W;
    if (dSquareSim > dTolerance) {
      const double scale = 1.0 / ::sqrt(dSquareSim);
      this->operator*=(scale);
      return true;
    }
    return false;
  }

  //对向量进行旋转，要求当前的四元数已经进行归一化
  FORCEINLINE Vector3D Rotate(const Vector3D& V) const {
    Quat QV(V);
    Quat Res = *this ^ QV ^ Conjugate();
    return Vector3D(Res.X, Res.Y, Res.Z);
  }
};

//总是以自身为参考系内旋，旋转顺序是Yaw->Pitch->Roll, 其中Yaw为左手系，Pitch和Roll为右手系
struct Rotator {  // 参考虚幻的代码
  /** Rotation around the right axis (around Y axis), Looking up and down (0=Straight Ahead, +Up, -Down) */
  double Pitch;
  /** Rotation around the up axis (around Z axis), Running in circles (0=Forward, +Right, -Left). */
  double Yaw;
  /** Rotation around the forward axis (around X axis), Tilting your head, 0=Straight, +Clockwise, -CCW. */
  double Roll;

  FORCEINLINE Rotator() : Pitch(0.0f), Yaw(0.0f), Roll(0.0f) {}

  explicit FORCEINLINE Rotator(double InF) : Pitch(InF), Yaw(InF), Roll(InF) {}

  FORCEINLINE Rotator(double InPitch, double InYaw, double InRoll) : Pitch(InPitch), Yaw(InYaw), Roll(InRoll) {}

  FORCEINLINE Rotator operator+(const Rotator& R) const { return Rotator(Pitch + R.Pitch, Yaw + R.Yaw, Roll + R.Roll); }

  FORCEINLINE Rotator operator-(const Rotator& R) const { return Rotator(Pitch - R.Pitch, Yaw - R.Yaw, Roll - R.Roll); }

  FORCEINLINE Rotator operator-() const { return Rotator(-Pitch, -Yaw, -Roll); }

  FORCEINLINE Rotator operator*(double dScale) const { return Rotator(Pitch * dScale, Yaw * dScale, Roll * dScale); }

  FORCEINLINE bool operator==(const Rotator& R) const { return Pitch == R.Pitch && Yaw == R.Yaw && Roll == R.Roll; }

  FORCEINLINE bool operator!=(const Rotator& R) const { return Pitch != R.Pitch || Yaw != R.Yaw && Roll != R.Roll; }

  FORCEINLINE Rotator operator+=(const Rotator& R) {
    Pitch += R.Pitch;
    Yaw += R.Yaw;
    Roll += R.Roll;
    return *this;
  }

  FORCEINLINE Rotator operator-=(const Rotator& R) {
    Pitch -= R.Pitch;
    Yaw -= R.Yaw;
    Roll -= R.Roll;
    return *this;
  }

  FORCEINLINE Rotator operator*=(double dScale) {
    Pitch *= dScale;
    Yaw *= dScale;
    Roll *= dScale;
    return *this;
  }

  // 转换角度到[0,360)
  FORCEINLINE static double ClampAxis(double dAngle) {
    dAngle = PhysMath::Mod(dAngle, 360.0);
    if (dAngle < 0) dAngle += 360;
    return dAngle;
  }

  // 转换角度到(-180,180]
  FORCEINLINE static double NormalizeAxis(double dAngle) {
    dAngle = ClampAxis(dAngle);
    if (dAngle > 180.0) dAngle -= 360;
    return dAngle;
  }

  // 各轴旋转角度转换到(-180,180]
  FORCEINLINE void Normalize() {
    Pitch = NormalizeAxis(Pitch);
    Yaw = NormalizeAxis(Yaw);
    Roll = NormalizeAxis(Roll);
  }

  FORCEINLINE bool IsNearlyZero(float Tolerance = KINDA_SMALL_NUMBER) const {
    return PhysMath::Abs(NormalizeAxis(Pitch)) <= Tolerance && PhysMath::Abs(NormalizeAxis(Yaw)) <= Tolerance && PhysMath::Abs(NormalizeAxis(Roll)) <= Tolerance;
  }

  FORCEINLINE bool IsZero() const { return (ClampAxis(Pitch) == 0.0) && (ClampAxis(Yaw) == 0.0) && (ClampAxis(Roll) == 0.0); }

  FORCEINLINE bool Equals(const Rotator& R, float Tolerance = KINDA_SMALL_NUMBER) const {
    return (PhysMath::Abs(NormalizeAxis(Pitch - R.Pitch)) <= Tolerance) && (PhysMath::Abs(NormalizeAxis(Yaw - R.Yaw)) <= Tolerance) && (PhysMath::Abs(NormalizeAxis(Roll - R.Roll)) <= Tolerance);
  }

  // 向前的向量， rotate处理后的情形, 其它两个轴，可以使用四元数rotate
  FORCEINLINE Vector3D VectorForward() const {
    const double PitchNoWinding = PhysMath::Mod(Pitch, 360.0);
    const double YawNoWinding = PhysMath::Mod(Yaw, 360.0);
    double CP, SP, CY, SY;
    PhysMath::SinCos(&SP, &CP, PhysMath::DegreesToRadians(PitchNoWinding));
    PhysMath::SinCos(&SY, &CY, PhysMath::DegreesToRadians(YawNoWinding));
    return Vector3D(CP * CY, CP * SY, SP);
  }

  FORCEINLINE Quat Quaternion() const {
    const double PitchNoWinding = PhysMath::Mod(Pitch, 360.0f);
    const double YawNoWinding = PhysMath::Mod(Yaw, 360.0f);
    const double RollNoWinding = PhysMath::Mod(Roll, 360.0f);

    const double RADS_DIVIDED_BY_2 = DEG_TO_RAD / 2.0;

    double SP, SY, SR;
    double CP, CY, CR;

    PhysMath::SinCos(&SP, &CP, PitchNoWinding * RADS_DIVIDED_BY_2);
    PhysMath::SinCos(&SY, &CY, YawNoWinding * RADS_DIVIDED_BY_2);
    PhysMath::SinCos(&SR, &CR, RollNoWinding * RADS_DIVIDED_BY_2);

    return Quat(CR * SP * SY - SR * CP * CY, -CR * SP * CY - SR * CP * SY, CR * CP * SY - SR * SP * CY, CR * CP * CY + SR * SP * SY);
  }
};

}