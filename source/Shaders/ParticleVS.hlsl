struct Particle
{
    float3 Position;
    float Density;
    float3 Velocity;
    float Pressure;
    float3 Force;
    float Padding;
};

StructuredBuffer<Particle> Particles : register(t0);

cbuffer Transform : register(b0)
{
    float4x4 Billboard; // ビルボード行列（Viewの逆行列の回転成分）
    float4x4 View;
    float4x4 Proj;
};

struct VSInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output = (VSOutput) 0;
    // パーティクルの位置を取得
    float3 particlePos = Particles[instanceID].Position;
    
    // 粒子のサイズ
    float particleSize = 0.1f;
    
    // ビルボード計算
    float3 worldPos = particlePos + mul((float3x3) Billboard, input.Position) * particleSize;
    float4 viewPos = mul(View, float4(worldPos, 1.0f));
    output.Position = mul(Proj, viewPos);
    
    output.TexCoord = input.TexCoord;
    
    float speed = length(Particles[instanceID].Velocity);
    
// デバッグ用: 密度可視化
    float density = Particles[instanceID].Density;

    float targetDensity = 1000.0f; // C++のRestDensityと合わせる
    float nDensity = saturate((density - targetDensity) / 200.0f); // 範囲を広げる

    // 青(低密度) -> 赤(高密度)
    output.Color = float4(lerp(float3(0, 0.5, 1), float3(1, 0, 0), nDensity), 1.0f);
    return output;

}