struct VSOutput
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

float4 main(VSOutput input) : SV_Target
{
    // UV座標 (0~1) を (-1~1) に変換
    float2 uv = input.TexCoord * 2.0f - 1.0f;
    
    // 中心からの距離の2乗
    float distSq = dot(uv, uv);
    
    if(distSq > 1.0f)
    {
        // 円の外側は透明にする
        discard;
    }
    
    // 中心ほど明るく
    float alpha = 1.0f - distSq;
    float3 color = input.Color.rgb * (0.2f + 0.8f * alpha);
    
    return float4(color, 1.0f);
}