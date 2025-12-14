#pragma once
namespace MathUtility
{
    static constexpr float PI = 3.14159265359f;
    static constexpr float DEG_TO_RAD = PI / 180.0f;

    /// <summary>
	/// 度をラジアンに変換します
    /// </summary>
    inline float DegreeToRadian(const float& deg)
    {
        return deg * DEG_TO_RAD;
    }

    /// <summary>
/// 度をラジアンに変換します
/// </summary>
    inline float RadianToDegree(const float& rad)
    {
        return rad / DEG_TO_RAD;
    }
}