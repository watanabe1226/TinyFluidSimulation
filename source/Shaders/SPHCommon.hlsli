#ifndef SPH_COMMON_HLSLI
#define SPH_COMMON_HLSLI

// 物理定数
static const float PI = 3.14159265359f;

struct Particle
{
    float3 Position;
    float Density;
    float3 Velocity;
    float Pressure;
    float3 Force;
    float NearDensity;
};

// 密度を計算する用のカーネル関数(重み)
// W(r, h) = (315 / (64 * pi * h^9)) * (h^2 - r^2)^3   (0 <= r <= h の場合)
// W(r, h) = 0                                         (r > h の場合)
inline float Poly6Kernel(float r, float h)
{
    if(r < h)
    {
        float h2 = h * h;
        float r2 = r * r;
        float term = h2 - r2;
        float coef = 315.0f / (64.0f * PI * pow(h, 9));
    
        return coef * term * term * term;
    }
    return 0.0f;
}

// 近傍密度を計算する用のカーネル関数(重み)
inline float NearDensityKernel(float r, float h)
{   
    if (r < h)
    {
        float term = h - r;
        float coef = 15.0f / (PI * pow(h, 6));
    
        return coef * term * term * term;
    }
    return 0.0f;
}

// 近傍圧力計算用のカーネル関数
inline float SpikyKernelGradient(float r, float h)
{
    if (r <= h)
    {
        float term = h - r;
        float coef = -45.0f / (PI * pow(h, 6));
        return coef * term * term;
    }
    return 0.0f;
}

// 圧力計算用のカーネル関数
inline float NearSpikyKernelGradient(float r, float h)
{   
    if (r <= h)
    {
        float term = h - r;
        float coef = -15.0f / (PI * pow(h, 5));
        return coef * term;
    }
    return 0.0f;
}

// 粘性計算用のカーネル（Laplacian）
// ∇^2W(r, h) = (45 / (pi * h^6)) * (h - r)
inline float ViscosityKernelLaplacian(float r, float h)
{
    if(r < h)
    {
        float coef = 45.0f / (PI * pow(h, 6));
        return coef * (h - r);
    }
    return 0.0f;
}

inline int3 GetGridPos(float3 pos, float3 wallMin, float gridH, int3 gridDim)
{
    return clamp(int3((pos - wallMin) / gridH), int3(0, 0, 0), gridDim - 1);
}

// 座標からグリッドのインデックスを取得
inline int GetGridIndex(int3 gridPos, int3 gridDim)
{
    return gridPos.x + gridPos.y * gridDim.x + gridPos.z * gridDim.x * gridDim.y;
}

#endif // SPH_COMMON_HLSLI