struct VSOutput
{
    float4 Position : SV_POSITION;
    float3 TexCoord : TEXCOORD;
};

struct PSOutput
{
    float4 Color : SV_TARGET;
};

SamplerState ColorSmp : register(s0);
TextureCube ColorMap : register(t0);

PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;
    
    output.Color = ColorMap.SampleLevel(ColorSmp, input.TexCoord, 0);
    
    return output;
}