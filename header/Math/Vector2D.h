#pragma once
#include "pch.h"

class Vector2D
{
public:
    // デフォルトコンストラクタ（(0,0,0)に初期化）
    Vector2D() noexcept = default;
    // XY指定コンストラクタ
    Vector2D(float x, float y) noexcept : x(x), y(y){}
    // 同じ値でXY初期化（explicitで暗黙変換防止）
    explicit Vector2D(float v) noexcept : x(v), y(v){}
    // コピーコンストラクタ
    Vector2D(const Vector2D&) noexcept = default;
    // 代入演算子
    Vector2D& operator=(const Vector2D&) noexcept = default;

    Vector2D operator+(const Vector2D& vec) const noexcept
    {
        return Vector2D(x + vec.x, y + vec.y);
    }

    Vector2D operator-(const Vector2D& vec) const noexcept
    {
        return Vector2D(x - vec.x, y - vec.y);
    }

    Vector2D operator*(float scalar) const noexcept
    {
        return Vector2D(x * scalar, y * scalar);
    }

    Vector2D& operator+=(const Vector2D& vec) noexcept
    {
        x += vec.x;
        y += vec.y;

        return *this;
    }

    Vector2D& operator-=(const Vector2D& vec) noexcept
    {
        x -= vec.x;
        y -= vec.y;

        return *this;
    }

    Vector2D& operator*=(const float s) noexcept
    {
        x *= s;
        y *= s;

        return *this;
    }

    Vector2D& operator*=(const Vector2D& vec) noexcept
    {
        x *= vec.x;
        y *= vec.y;

        return *this;
    }

    Vector2D& operator/=(const float s)
    {
        return *this *= (1.0f / s);
    }

    Vector2D& operator/=(const Vector2D& v)
    {
        x /= v.x;
        y /= v.y;

        return *this;
    }

    float dot(const Vector2D& vec) const noexcept
    {
        return x * vec.x + y * vec.y;
    }

    float length() const noexcept
    {
        return std::sqrt(dot(*this));
    }

    Vector2D cross(const Vector2D& vec) const noexcept
    {
        return Vector2D(x * vec.y - y * vec.x);
    }

    Vector2D GetSafeNormal() const noexcept
    {
        const float Size = length();
        return (Size > SMALL_NUMBER) ? (*this * (1.f / Size)) : Vector2D(0.f);
    }

public:
    float x = 0.0f, y = 0.0f;
};