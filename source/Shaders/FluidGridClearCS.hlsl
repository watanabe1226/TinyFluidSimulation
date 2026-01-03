#include "SPHCommon.hlsli"

RWStructuredBuffer<Particle> Particles : register(u0);
RWStructuredBuffer<int> GridHead : register(u1);
RWStructuredBuffer<int> GridNext : register(u2);

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

// グリッドのリセット
[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    if(id >= gridCount * 3) return;
    
    // -1で初期化
    GridHead[id] = -1;
}