cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_INPUT
{
    float4 Pos : POSITION;
    float3 Norm : NORMAL;
    float2 Tex : TEXCOORD0;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 PosL : POSITION;
};

PS_INPUT VS(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    output.Pos = mul(output.Pos, Projection).xyww;

	// Use local vertex position as cubemap lookup vector.
    output.PosL = (float3) input.Pos;

    return output;
}