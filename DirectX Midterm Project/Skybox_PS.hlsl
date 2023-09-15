TextureCube txDiffuse : register(t0);
SamplerState samLinear : register(s0);

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float3 PosL : POSITION;
};

float4 PS(PS_INPUT input) : SV_TARGET
{
    return  txDiffuse.Sample(samLinear, input.PosL);
}