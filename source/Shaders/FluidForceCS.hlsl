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
    float3 pressureForce = float3(0, 0, 0);
    float3 viscosityForce = float3(0, 0, 0);
    float3 myPosition = Particles[id].Position;
    float myPressure = Particles[id].Pressure;
    float3 myVelocity = Particles[id].Velocity;
    float myNearDensity = Particles[id].NearDensity;
    float myNearPressure = nearStiffness * myNearDensity;
    // 圧力項、粘性項の計算
    for (int i = 0; i < ParticleCount; ++i)
    {
        if (id == i)
            continue;
        float3 otherPos = Particles[i].Position;
        float3 diff = myPosition - otherPos;
        float r2 = dot(diff, diff);
        float3 dir = normalize(diff);
        // 影響範囲外チェック
        float h2 = H * H;
        if (r2 >= h2 || r2 == 0.0f)
            continue;
        float r = sqrt(r2);
        float otherDensity = Particles[i].Density;
        float otherPressure = Particles[i].Pressure;
        float3 otherVelocity = Particles[i].Velocity;
        float otherNearDensity = Particles[i].NearDensity;
        
        if (otherDensity == 0.0f)
            continue;
        // 圧力項
        float sharedPressure = (myPressure + otherPressure) / 2.0f;
        pressureForce += -Mass * sharedPressure * dir * SpikyKernelGradient(r, H) / otherDensity;
        
        // 粘性項
        float3 relativeSpeed = otherVelocity - myVelocity;
        viscosityForce += Mass * relativeSpeed * ViscosityKernelLaplacian(r, H) / otherDensity;
            
        if (otherNearDensity == 0.0f)
            continue;
        // 近傍圧力
        float otherNearPressure = nearStiffness * otherNearDensity;
        float sharedNearPressure = (myNearPressure + otherNearPressure) / 2.0f;
        pressureForce += -Mass * sharedNearPressure * dir * NearSpikyKernelGradient(r, H) / otherNearDensity;
    };

    // 力の合成
    float3 gravityVec = float3(0.0f, Gravity, 0.0f);
    float3 externalForce =  Particles[id].Density * gravityVec;
    viscosityForce *= Viscosity;
    Particles[id].Force = pressureForce + viscosityForce + externalForce;
}