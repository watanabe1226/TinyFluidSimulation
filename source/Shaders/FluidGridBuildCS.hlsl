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

// pass2 パーティクルをグリッドに登録
[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    if (id >= ParticleCount) return;
    
    float3 pos = Particles[id].Position;
    int3 gridDim = int3(gridCount, gridCount, gridCount);
    // 自分がどのグリッドにいるかどうか計算
    int3 gridPos = GetGridPos(pos, WallMin, H, gridDim);
    int gridIndex = GetGridIndex(gridPos, gridDim);
    
    // リンクリストへの挿入
    int originalHead;
    // GridHead[gridIndex]に自身のidを書き込み、古い値をoriginalHeadに受け取る
    InterlockedExchange(GridHead[gridIndex], id, originalHead);
    
    // 古いheadを自分のnextに設定
    GridNext[id] = originalHead;

}