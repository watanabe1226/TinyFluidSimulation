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
    float Padding0;
    float3 GridDim;
    float Padding1;
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
    int3 myGridPos = GetGridPos(myPosition, WallMin, H);
    // 圧力項、粘性項の計算
    for (int z = -1; z <= 1; ++z)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                int3 neighborGridPos = myGridPos + int3(x, y, z);
                int gridIndex = GetGridIndex(neighborGridPos, GridDim);
                if (gridIndex == -1)
                {
                    continue;
                }
                int neighborId = GridHead[gridIndex];
                while (neighborId != -1)
                {
                    if (id == neighborId)
                    {
                        neighborId = GridNext[neighborId];
                        continue;
                    }
                    float3 otherPos = Particles[neighborId].Position;
                    float3 diff = myPosition - otherPos;
                    float r2 = dot(diff, diff);
                    float3 dir = normalize(diff);
                    // 影響範囲外チェック
                    float h2 = H * H;
                    if (r2 >= h2 || r2 < 0.00001f)
                    {
                        neighborId = GridNext[neighborId];
                        continue;
                    }
                    float r = sqrt(r2);
                    float otherDensity = Particles[neighborId].Density;
                    float otherPressure = Particles[neighborId].Pressure;
                    float3 otherVelocity = Particles[neighborId].Velocity;
                    float otherNearDensity = Particles[neighborId].NearDensity;
        
                    if (otherDensity == 0.0f || otherNearDensity == 0.0f)
                    {
                        neighborId = GridNext[neighborId];
                        continue;
                    }
                    // 圧力項
                    float sharedPressure = (myPressure + otherPressure) / 2.0f;
                    pressureForce += -Mass * sharedPressure * dir * SpikyKernelGradient(r, H) / otherDensity;
        
                    // 粘性項
                    float3 relativeSpeed = otherVelocity - myVelocity;
                    viscosityForce += Mass * relativeSpeed * ViscosityKernelLaplacian(r, H) / otherDensity;

                    // 近傍圧力
                    float otherNearPressure = nearStiffness * otherNearDensity;
                    float sharedNearPressure = (myNearPressure + otherNearPressure) / 2.0f;
                    pressureForce += -Mass * sharedNearPressure * dir * NearSpikyKernelGradient(r, H) / otherNearDensity;
                    
                    neighborId = GridNext[neighborId];
                }
            }
        }
    }

    // 力の合成
    float3 gravityVec = float3(0.0f, Gravity, 0.0f);
    float3 externalForce = float3(0.0f, 0.0f, 0.0f);
    externalForce = Particles[id].Density * gravityVec;
    viscosityForce *= Viscosity;
    Particles[id].Force = pressureForce + viscosityForce + externalForce;
}