struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float3 ray : VECTOR;
    float4 tPos : TPOS;
    float3 WorldPos : WORLD_POS;
    float3x3 InvTangentBasis : INV_TANGENT_BASIS; // 接線空間への基底変換行列の逆行列
};

struct PSOutput
{
    float4 Color : SV_TARGET;

};

cbuffer Material : register(b2)
{
    float3 Difuuse;
    float Alpha;
    float3 Specular;
    float Shininess;
}

cbuffer LightTransform : register(b3)
{
    float4x4 LightVP;
    float3 LightDir;
    float Padding;
}

SamplerState ColorSmp : register(s0);
SamplerState NormalSmp : register(s1);
SamplerState GLTFMetallicRoughnessSmp : register(s2);
SamplerState DFGSmp : register(s3);
SamplerState DiffuseLDSmp : register(s4);
SamplerState SpecularLDSmp : register(s5);
SamplerComparisonState ShadowSmp : register(s6);

Texture2D ColorMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D GLTFMetallicRoughnessMap : register(t2);
Texture2D DFGMap : register(t3);
TextureCube DiffuseLDMap : register(t4);
TextureCube SpecularLDMap : register(t5);
Texture2D ShadowMap : register(t6);

// スペキュラーの支配的な方向を求めます
float3 GetSpecularDomiantDir(float3 N, float3 R, float roughness)
{
    float smoothness = saturate(1.0f - roughness);
    float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
    return lerp(N, R, lerpFactor);
}

// ディフューズIBLを評価します.
float3 EvaluateIBLDiffuse(float3 N)
{
    // Lambert BRDFはDFG項は積分すると1.0となるので，LD項のみを返却すれば良い
    return DiffuseLDMap.Sample(DiffuseLDSmp, N).rgb;
}

// 線形ラフネスからミップレベルを求めます.
float RoughnessToMipLevel(float linearRoughness, float mipCount)
{
    return (mipCount - 1) * linearRoughness;
}

// スペキュラーIBLを評価します.
float3 EvaluateIBLSpecular
(
    float NdotV, // 法線ベクトルと視線ベクトルの内積.
    float3 N, // 法線ベクトル.
    float3 R, // 反射ベクトル.
    float3 f0, // フレネル項
    float roughness, // 線形ラフネス.
    float textureSize, // テクスチャサイズ.
    float mipCount // ミップレベル数.
)
{
    float a = roughness * roughness;
    float3 dominantR = GetSpecularDomiantDir(N, R, a);

    // 関数を再構築.
    // L * D * (f0 * Gvis * (1 - Fc) + Gvis * Fc) * cosTheta / (4 * NdotL * NdotV).
    NdotV = max(NdotV, 0.5f / textureSize); // ゼロ除算が発生しないようにする.
    float mipLevel = RoughnessToMipLevel(roughness, mipCount);
    float3 preLD = SpecularLDMap.SampleLevel(SpecularLDSmp, dominantR, mipLevel).xyz;
    
    // 事前積分したDFGをサンプルする.
    // Fc = ( 1 - HdotL )^5
    // PreIntegratedDFG.r = Gvis * (1 - Fc)
    // PreIntegratedDFG.g = Gvis * Fc
    float2 preDFG = DFGMap.SampleLevel(DFGSmp, float2(NdotV, roughness), 0).xy;

    // LD * (f0 * Gvis * (1 - Fc) + Gvis * Fc)
    return preLD * (f0 * preDFG.x + preDFG.y);
}

PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;
    
    float3 V = input.ray;
    float3 N = NormalMap.Sample(NormalSmp, input.TexCoord).xyz * 2.0f - 1.0f;
    N = mul(input.InvTangentBasis, N);
    float3 R = normalize(reflect(V, N));

    float NV = saturate(dot(N, V));
    
    float4 baseColor = ColorMap.Sample(ColorSmp, input.TexCoord);
    float3 MR = GLTFMetallicRoughnessMap.Sample(GLTFMetallicRoughnessSmp, input.TexCoord).rgb;
    float metallic = MR[1];
    float roughness = MR[2];
    
    float3 Kd = baseColor * (1.0f - metallic);
    float3 Ks = baseColor * metallic;
    
    float3 lit = 0;
    lit += EvaluateIBLDiffuse(N) * Kd;
    float LDTextureSize = 128.0f;
    float MipCount = 7.0f;
    lit += EvaluateIBLSpecular(NV, N, R, Ks, roughness, LDTextureSize, MipCount);
    
    float3 posFromLightVP = input.tPos.xyz / input.tPos.w;
    float2 shadowUV = (posFromLightVP.xy + float2(1, -1)) * float2(0.5f, -0.5f);
    float depthFromLight = ShadowMap.SampleCmp(ShadowSmp, shadowUV, posFromLightVP.z - 0.0005f);
    float shadowWeight = lerp(0.5f, 1.0f, depthFromLight);
    
    output.Color = float4(lit * shadowWeight, 1.0f);
    
    return output;
}