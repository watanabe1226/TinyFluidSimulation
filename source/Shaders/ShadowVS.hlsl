struct VSInput
{
    float3 Position : POSITION;
};

struct VSOutput
{
    float4 Position : SV_POSITION;
};

cbuffer Transform : register(b0)
{
    float4x4 LightVP : packoffset(c0);
    float4x4 ModelWorld : packoffset(c4);
}

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    
    float4 localPos = float4(input.Position, 1.0f);
    float4 worldPos = mul(ModelWorld, localPos);
    float4 projPos = mul(LightVP, worldPos);
    
    output.Position = projPos;
    
    return output;
}