#include "ShaderConstants.h"

struct VERTEX_DATA
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : UV;
    uint instance_id : SV_InstanceID;
};

struct VERTEX_DATA_ENVIRONMENT
{
    float3 Position : POSITION;
    uint instance_id : SV_InstanceID;
};

cbuffer CONSTANT_VS : register(b0)
{
    matrix<float, 4, 4> ModelMatrix;
    matrix<float, 4, 4> ViewMatrix[2]; // left and right
    matrix<float, 4, 4> ProjectionMatrix[2];
    matrix<float, 4, 4> NormalMatrix;
};