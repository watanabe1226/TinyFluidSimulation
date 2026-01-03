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
void main( uint3 DTid : SV_DispatchThreadID )
{
    uint id = DTid.x;
    if (id >= ParticleCount)
        return;
    
    if(Particles[id].Density != 0.0f)
    {
        float3 acceleration = Particles[id].Force / Particles[id].Density;

        // 1. ボックスの「半分のサイズ」と「中心座標」を計算
        float3 halfRealBoxSize = (WallMax - WallMin) / 2.0f;
        float3 boxCenter = (WallMax + WallMin) / 2.0f;

        // 2. 粒子の座標を「ボックス中心からの相対座標」に変換
        float3 localPos = Particles[id].Position - boxCenter;

        float xPlusDist = halfRealBoxSize.x - localPos.x;
        float xMinusDist = halfRealBoxSize.x + localPos.x;
        float yPlusDist = halfRealBoxSize.y - localPos.y;
        float yMinusDist = halfRealBoxSize.y + localPos.y;
        float zPlusDist = halfRealBoxSize.z - localPos.z;
        float zMinusDist = halfRealBoxSize.z + localPos.z;

        float wallStiffness = 1000.0f;

        // ベクトル計算
        float3 force = float3(0, 0, 0);
        
        // X軸
        force.x += 1.0f * wallStiffness * min(xPlusDist, 0); // 右壁からの反発(左へ)
        force.x += -1.0f * wallStiffness * min(xMinusDist, 0); // 左壁からの反発(右へ)

        // Y軸
        force.y += 1.0f * wallStiffness * min(yPlusDist, 0);
        force.y += -1.0f * wallStiffness * min(yMinusDist, 0);

        // Z軸
        force.z += 1.0f * wallStiffness * min(zPlusDist, 0);
        force.z += -1.0f * wallStiffness * min(zMinusDist, 0);
        
        acceleration += force;

        // 速度と位置の更新
        Particles[id].Velocity += acceleration * DeltaTime;
        Particles[id].Position += Particles[id].Velocity * DeltaTime;
    }
}