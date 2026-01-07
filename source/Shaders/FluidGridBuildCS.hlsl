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

// pass2 パーティクルをグリッドに登録
[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    if (id >= ParticleCount) return;
    
    float3 pos = Particles[id].Position;
    // 自分がどのグリッドにいるかどうか計算
    int3 gridPos = GetGridPos(pos, WallMin, H);
    int gridIndex = GetGridIndex(gridPos, GridDim);
    
    // リンクリストへの挿入
    if(gridIndex != -1)
    {
        int oldHead;
        // GridHead[gridIndex]に自身のidを書き込み、古い値をoldHeadに受け取る
        InterlockedExchange(GridHead[gridIndex], id, oldHead);
        // 古いheadを自分のnextに設定
        GridNext[id] = oldHead;
    }
    else
    {
        GridNext[id] = -1;
    }

}