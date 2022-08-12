#include "VertexShaderStructures.hlsli"

struct VS_OUTPUT
{
    float4 sv_position : SV_POSITION;
    uint view_id : SV_RenderTargetArrayIndex;
};

VS_OUTPUT main(VERTEX_DATA_ENVIRONMENT input)
{
    VS_OUTPUT output;

    int idx = input.instance_id % 2;

    output.sv_position = float4(input.Position, 1.0);
    output.sv_position = mul(ModelMatrix, output.sv_position);
    output.sv_position = mul(ViewMatrix[idx], output.sv_position);
    output.sv_position = mul(ProjectionMatrix[idx], output.sv_position);
    output.view_id = input.instance_id;

    return output;
}