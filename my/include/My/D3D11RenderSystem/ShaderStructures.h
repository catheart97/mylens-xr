#pragma once

#include "pch.h"

#include "My/D3D11RenderSystem/Shader/ShaderConstants.h"

namespace My::D3D11RenderSystem
{

namespace _implementation
{

using namespace winrt;
using namespace DirectX;

/**
 * @brief Vertex data layout.
 *
 * Struct describing the default 3 Vertex 3 Normal Layout for the GPU. The Corresponding
 * descriptor is inside of the Material class.
 *
 * @ingroup D3D11RenderSystem
 * @author  Ronja Schnur (rschnur@students.uni-mainz.de)
 */
struct VERTEX_DATA
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 UV;
};

struct VERTEX_DATA_ENVIRONMENT
{
    XMFLOAT3 Position;
};

/**
 * @brief   Constant buffer for vertex shaders.
 *
 * Struct describing the layout that all vertex shaders must accept as first (aka 0) constant buffer
 *
 * @ingroup D3D11RenderSystem
 * @author  Ronja Schnur (rschnur@students.uni-mainz.de)
 */
struct CONSTANT_VS
{
    XMMATRIX ModelMatrix;
    XMMATRIX ViewMatrix[2];
    XMMATRIX ProjectionMatrix[2];
    XMMATRIX NormalMatrix;
};

static_assert((sizeof(CONSTANT_VS) % (sizeof(float) * 4)) == 0,
              "Constant buffer size must be 16-byte aligned.");

} // namespace _implementation

using _implementation::CONSTANT_VS;
using _implementation::VERTEX_DATA;

} // namespace My::Renderer