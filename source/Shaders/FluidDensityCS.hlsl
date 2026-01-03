#include "SPHCommon.hlsli"

RWStructuredBuffer<Particle> Particles : register(u0);
RWStructuredBuffer<uint> GridHead : register(u1);
RWStructuredBuffer<uint> GridNext : register(u2);

cbuffer SimulationParam : register(b0)
{
    float DeltaTime;
    float Gravity;
    float Stiffness;
    float nearStiffness;
    uint ParticleCount;
    float3 WallMin;
    float RestDensity;
    float3 WallMax;
    float Viscosity;
    float H;
    float Mass;
    uint gridCount;
}

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    if (id >= ParticleCount)
        return;
    
    // 自身の座標
    float3 myPosition = Particles[id].Position;
    // 初期密度
    float density = 0.0f;
    float nearDensity = 0.0f;
    float h2 = H * H;
    // 近傍探索
    for (uint i = 0; i < ParticleCount; ++i)
    {                    
        float3 otherPosition = Particles[i].Position;
        float3 diff = myPosition - otherPosition;
        float dist2 = dot(diff, diff);
        float r = sqrt(dist2);
        // 影響範囲「外」ならスキップ
        if (dist2 >= h2) 
            continue;
        
        float W = Poly6Kernel(r, H);
        float nearW = NearDensityKernel(r, H);
        density += Mass * W;
        nearDensity += Mass * nearW;
    }

    Particles[id].Density = density;
    Particles[id].NearDensity = nearDensity;
    
    // 圧力 Tait方程式
    //float pressure = Stiffness * max(pow(density / RestDensity, 7) - 1, 0);
    float pressure = Stiffness * (density - RestDensity);
    Particles[id].Pressure = pressure;
}