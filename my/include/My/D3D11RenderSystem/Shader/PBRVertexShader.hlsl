#include "VertexShaderStructures.hlsli"

struct VS_OUTPUT
{
    float4 sv_position : SV_POSITION;
    float4 Position : POSITION;
    float4 Normal : NORMAL;
    float2 tex : TEXCOORD1;

    uint view_id : SV_RenderTargetArrayIndex;
};

VS_OUTPUT main(VERTEX_DATA input)
{
    VS_OUTPUT output;

    int idx = input.instance_id % 2;

    output.Position = float4(input.Position, 1.0);
    output.Position = mul(ModelMatrix, output.Position);
    output.sv_position = mul(ViewMatrix[idx], output.Position);
    output.sv_position = mul(ProjectionMatrix[idx], output.sv_position);
    output.Normal = mul(NormalMatrix, float4(input.Normal, 1.0));
    output.tex = input.UV;

    output.view_id = input.instance_id;

    return output;
}