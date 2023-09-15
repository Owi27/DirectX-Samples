struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
};

//--------------------------------------------------------------------------------------
// PSDebug - render a vertex color
//--------------------------------------------------------------------------------------
float4 PSDebug(PS_INPUT input) : SV_Target
{
    return input.Color;
}
