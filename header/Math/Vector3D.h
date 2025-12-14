#pragma once
#include "pch.h"

class Vector3D
{
public:
    // デフォルトコンストラクタ（(0,0,0)に初期化）
    Vector3D() noexcept = default;
    // XYZ指定コンストラクタ
    Vector3D(float x, float y, float z) noexcept : x(x), y(y), z(z) {}
    // 同じ値でXYZ初期化（explicitで暗黙変換防止）
    explicit Vector3D(float v) noexcept : x(v), y(v), z(v) {}
    // コピーコンストラクタ
    Vector3D(const Vector3D&) noexcept = default;
    // 代入演算子
    Vector3D& operator=(const Vector3D&) noexcept = default;

    Vector3D operator+(const Vector3D& vec) const noexcept
    {
        return Vector3D(x + vec.x, y + vec.y, z + vec.z);
    }

    Vector3D operator-(const Vector3D& vec) const noexcept
    {
        return Vector3D(x - vec.x, y - vec.y, z - vec.z);
    }

    Vector3D operator*(float scalar) const noexcept
    {
        return Vector3D(x * scalar, y * scalar, z * scalar);
    }

    Vector3D& operator+=(const Vector3D& vec) noexcept
    {
        x += vec.x;
        y += vec.y;
        z += vec.z;

        return *this;
    }

    Vector3D& operator-=(const Vector3D& vec) noexcept
    {
        x -= vec.x;
        y -= vec.y;
        z -= vec.z;

        return *this;
    }

    Vector3D& operator*=(const float s) noexcept
    {
        x *= s;
        y *= s;
        z *= s;

        return *this;
    }

    Vector3D& operator*=(const Vector3D& vec) noexcept
    {
        x *= vec.x;
        y *= vec.y;
        z *= vec.z;

        return *this;
    }

    Vector3D& operator/=(const float s)
    {
        return *this *= (1.0f / s);
    }

    Vector3D& operator/=(const Vector3D& v)
    {
        x /= v.x;
        y /= v.y;
        z /= v.z;

        return *this;
    }

    float dot(const Vector3D& vec) const noexcept
    {
        return x * vec.x + y * vec.y + z * vec.z;
    }

    float length() const noexcept
    {
        return std::sqrt(dot(*this));
    }

    Vector3D cross(const Vector3D& vec) const noexcept
    {
        return Vector3D(
            y * vec.z - z * vec.y,
            z * vec.x - x * vec.z,
            x * vec.y - y * vec.x
        );
    }

    Vector3D GetSafeNormal() const noexcept
    {
        const float Size = length();
        return (Size > SMALL_NUMBER) ? (*this * (1.f / Size)) : Vector3D(0.f);
    }

public:
    float x = 0.0f, y = 0.0f, z = 0.0f;
};