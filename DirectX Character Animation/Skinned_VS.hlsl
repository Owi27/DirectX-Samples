//--------------------------------------------------------------------------------------
// File: Tutorial06.fx
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------

cbuffer ConstantBufferTransforms : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

cbuffer joint_deltas_t : register(b1)
{
    matrix m[67];
};


//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float4 pos : POSITION;
    float3 norm : NORMAL;
    float2 Tex : TEXCOORD0;
    float4 weights : BLENDWEIGHTS;
    int4 indices : BLENDINDICES;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 norm : NORMAL;
    float2 Tex : TEXCOORD1;
};


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT VS(VS_INPUT input)
{    
    PS_INPUT output;
    bool skinning_on = false;
    if (skinning_on)
    {
    // skinning VS shader
    float4 skinned_pos = { 0.0f, 0.0f, 0.0f, 0.0f };
    float4 skinned_norm = { 0.0f, 0.0f, 0.0f, 0.0f };

	[unroll]
    for (int j = 0; j < 4; j++)
    {
        skinned_pos += mul(float4(input.pos.xyz, 1.0f), m[input.indices[j]]) * input.weights[j];
        skinned_norm += mul(float4(input.norm.xyz, 0.0f), m[input.indices[j]]) * input.weights[j];
    }
    output.pos = mul(float4(skinned_pos.xyz, 1.0f), World);
    output.pos = mul(output.pos, View);
    output.pos = mul(output.pos, Projection);
    output.norm = mul(float4(skinned_norm.xyz, 0.0f), World);    
    output.Tex = input.Tex;
    }
    else
    {
    // regular VS shader    
    input.pos.xyz *= 0.75f;
    output.pos = mul(float4(input.pos.xyz, 1.0f), World);
    output.pos = mul(output.pos, View);
    output.pos = mul(output.pos, Projection);
    output.norm = mul(float4(input.norm.xyz, 0.0f), World);
    output.Tex = input.Tex;
    }
    
    return output;
}
