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
    
    // 自身の座標
    float3 myPosition = Particles[id].Position;
    int3 myGridPos = GetGridPos(myPosition, WallMin, H);
    // 初期密度
    float density = 0.0f;
    float nearDensity = 0.0f;
    float h2 = H * H;
    // 近傍探索
    for(int z = -1; z <= 1; ++z)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                int3 neighborGridPos = myGridPos + int3(x, y, z);
                int gridIndex = GetGridIndex(neighborGridPos, GridDim);
                if(gridIndex == -1)
                {
                    continue;
                }
                int neighborId = GridHead[gridIndex];
                while(neighborId != -1)
                {
                    float3 otherPosition = Particles[neighborId].Position;
                    float3 diff = myPosition - otherPosition;
                    float r2 = dot(diff, diff); // 距離の2乗
                    float r = sqrt(r2); // 必要な場合のみルート計算
                    float W = Poly6Kernel(r, H);
                    float nearW = NearDensityKernel(r, H);
                    density += Mass * W;
                    nearDensity += Mass * nearW;
                    
                    neighborId = GridNext[neighborId];
                }
            }
        }
    }
    if(density == 0.0f)
    {
        density = 0.0000001f;
    }
    Particles[id].Density = density;
    Particles[id].NearDensity = nearDensity;
    
    // 圧力 状態方程式
    float densityError = density - RestDensity;
    Particles[id].Pressure = Stiffness * densityError;
}