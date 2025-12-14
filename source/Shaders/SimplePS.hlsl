struct VSOutput
{
	float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

struct PSOutput
{
    float4 Color : SV_TARGET;

};

PSOutput main(VSOutput input)
{
    PSOutput output = (PSOutput) 0;
   
    output.Color = input.Color;
    
    return output;
}