#pragma once
#include "pch.h"
#include "Matrix4x4.h"
#include "Vector3D.h"
#include "Math/MathUtility.h"

class Quaternion
{
public:
    Quaternion() noexcept = default;
    constexpr Quaternion(float v_x, float v_y, float v_z, float v_w) noexcept : x(v_x), y(v_y), z(v_z), w(v_w) {}
    // explicit 意図せずにfloatがVector4D型に変更されないため
    explicit constexpr Quaternion(float v) noexcept : x(v), y(v), z(v), w(v) {}
    explicit constexpr Quaternion(const Vector3D& v) noexcept : x(v.x), y(v.y), z(v.z), w(1.0f) {}

    Quaternion& operator+=(const Quaternion& q) noexcept
    {
        x += q.x;
        y += q.y;
        z += q.z;
        w += q.w;
        return *this;
    }
    Quaternion& operator-=(const Quaternion& q) noexcept
    {
        x -= q.x;
        y -= q.y;
        z -= q.z;
        w -= q.w;
        return *this;
    }
    Quaternion& operator*=(float scale) noexcept
    {
        x *= scale;
        y *= scale;
        z *= scale;
        w *= scale;
        return *this;
    }
    Quaternion& operator*=(const Quaternion& q)
    {
        float x1 = x;
        float y1 = y;
        float z1 = z;
        float w1 = w;

        float x2 = q.x;
        float y2 = q.y;
        float z2 = q.z;
        float w2 = q.w;

        x = w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2;
        y = w1 * y2 + y1 * w2 + z1 * x2 - x1 * z2;
        z = w1 * z2 + z1 * w2 + x1 * y2 - y1 * x2;
        w = w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2;

        return *this;
    }

    float dot(const Quaternion& q) const
    {
        return w * q.w + x * q.x + y * q.y + z * q.z;
    }
    float length() const
    {
        return std::sqrt(dot(*this));
    }
    Quaternion& normalize()
    {
        float iLen = 1 / length();
        w *= iLen;
        x *= iLen;
        y *= iLen;
        z *= iLen;
        return *this;
    }

    static const Quaternion Euler(float x, float y, float z)
    {
        float radX = MathUtility::DegreeToRadian(x) * 0.5f;
        float radY = MathUtility::DegreeToRadian(y) * 0.5f;
        float radZ = MathUtility::DegreeToRadian(z) * 0.5f;

        float cx = std::cos(radX);
        float sx = std::sin(radX);
        float cy = std::cos(radY);
        float sy = std::sin(radY);
        float cz = std::cos(radZ);
        float sz = std::sin(radZ);

        // Z * X * Y の合成計算
        return Quaternion(
            sx * cy * cz + cx * sy * sz, // x
            cx * sy * cz - sx * cy * sz, // y
            cx * cy * sz - sx * sy * cz, // z
            cx * cy * cz + sx * sy * sz  // w
        );
    }

    static const Quaternion Euler(const Vector3D& vec)
    {
        return Euler(vec.x, vec.y, vec.z);
    }

    static const Vector3D EulerAngles(const Quaternion& q)
    {
        Vector3D angles;

        // X軸(Pitch)の計算
        float sinP = 2.0f * (q.w * q.x - q.y * q.z);
        if (std::abs(sinP) >= 1.0f)
            angles.x = std::copysign(MathUtility::PI / 2.0f, sinP); // 90度でクランプ
        else
            angles.x = std::asin(sinP);

        // Y軸(Yaw)とZ軸(Roll)の計算
        // ジンバルロック回避のため条件分岐する場合もあるが、asinの結果に基づく一般的な計算
        angles.y = std::atan2(2.0f * (q.w * q.y + q.x * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y));
        angles.z = std::atan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.x * q.x + q.z * q.z));

        // Radian -> Degree
        angles.x = MathUtility::RadianToDegree(angles.x);
        angles.y = MathUtility::RadianToDegree(angles.y);
        angles.z = MathUtility::RadianToDegree(angles.z);

        return angles;
    }

    //! @brief 指定した角度と回転軸からクォータニオンを生成
    //! @param[in] angle 角度
    //! @param[in] axis 回転軸
    //! @return クォータニオン
    static const Quaternion AngleAxis(float angle, const Vector3D& axis)
    {
        Vector3D normalizedAxis = axis.GetSafeNormal();
        float radian = MathUtility::DegreeToRadian(angle);
        float halfAngle = radian * 0.5f;
        float s = std::sin(halfAngle);

        return Quaternion(
            normalizedAxis.x * s,
            normalizedAxis.y * s,
            normalizedAxis.z * s,
            std::cos(halfAngle)
        );
    }

public:
    float x, y, z, w;
};

inline constexpr const Quaternion operator*(const Quaternion& vec1, const Quaternion& vec2) noexcept;

inline constexpr const Quaternion operator*(const Quaternion& vec1, const Quaternion& vec2) noexcept
{
    float w1 = vec1.w;
    float x1 = vec1.x;
    float y1 = vec1.y;
    float z1 = vec1.z;

    float w2 = vec2.w;
    float x2 = vec2.x;
    float y2 = vec2.y;
    float z2 = vec2.z;

    return Quaternion(
        w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
        w1 * y2 + y1 * w2 + z1 * x2 - x1 * z2,
        w1 * z2 + z1 * w2 + x1 * y2 - y1 * x2,
        w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2
    );
}