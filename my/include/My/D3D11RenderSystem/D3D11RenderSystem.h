#pragma once

#include "pch.h"

#include "My/Engine/ComponentManager.h"
#include "My/Engine/EntityManager.h"
#include "My/Engine/MaterialStructures.h"
#include "My/Engine/System.h"

#include "My/D3D11RenderSystem/HLSLShaderProgram.h"
#include "My/D3D11RenderSystem/Shader/ShaderConstants.h"
#include "My/D3D11RenderSystem/ShaderStructures.h"
#include "My/D3D11RenderSystem/Tools.h"

#include "My/Utility/winrtUtility.h"

namespace My::D3D11RenderSystem
{

namespace _implementation
{

using namespace My::Engine;

struct CONSTANT_PS_PBR
{
    Color albedo;
    float alpha;     // if < 0 => Use Texture
    float roughness; // if < 0 => Use Texture
    float ior;
    float ambient_occlusion;
    float metalness; // if < 0 => Use Texture
    XMVECTOR lights[MAX_NUMBER_LIGHTS];
    DirectX::XMFLOAT3 camera;
    float normal_map{0};
};

static_assert((sizeof(CONSTANT_PS_PBR) % (sizeof(float) * 4)) == 0,
              "Constant buffer size must be 16-byte aligned.");

struct CONSTANT_PS_PHONG
{
    Color ambient;                      // 3
    Color diffuse;                      // 3
    Color specular;                     // 3
    float alpha;                        // 1
    float shininess;                    // 1
    XMFLOAT3 camera;                    // 3
    XMVECTOR lights[MAX_NUMBER_LIGHTS]; // x * 4
    float dummy0, dummy1;
};

static_assert((sizeof(CONSTANT_PS_PHONG) % (sizeof(float) * 4)) == 0,
              "Constant buffer size must be 16-byte aligned.");

const std::vector<D3D11_INPUT_ELEMENT_DESC> DEFAULT_SHADER_LAYOUT_DESC{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 3 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA,
     0},
    {"UV", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, 5 * sizeof(float),
     D3D11_INPUT_PER_VERTEX_DATA, 0},
};

class D3D11RenderSystem : public System
{
    // Data //
private:
    winrt::com_ptr<ID3D11Device1> _device{nullptr};
    winrt::com_ptr<ID3D11DeviceContext1> _device_context{nullptr};

    // TODO: Double sided material states
    winrt::com_ptr<ID3D11RasterizerState1> _rasterizer_state_solid;
    winrt::com_ptr<ID3D11RasterizerState1> _rasterizer_state_wf;

    ID3D11Buffer * _constant_vs_buffer{nullptr};

    winrt::com_ptr<ID3D11DepthStencilState> _reversed_zdepth_no_stencil_test{nullptr};

    std::vector<winrt::com_ptr<ID3D11Buffer>> _vertex_buffers;
    std::vector<winrt::com_ptr<ID3D11Buffer>> _index_buffers;

    std::unique_ptr<HLSLShaderProgram> _pbr_shader{nullptr};
    winrt::com_ptr<ID3D11InputLayout> _pbr_input_layout;
    ID3D11Buffer * _pbr_constant_buffer{nullptr};

    std::unique_ptr<HLSLShaderProgram> _phong_shader{nullptr};
    winrt::com_ptr<ID3D11InputLayout> _phong_input_layout;
    ID3D11Buffer * _phong_constant_buffer{nullptr};

    // Properties
public:
    winrt::com_ptr<ID3D11Device1> Device() { return _device; }

    winrt::com_ptr<ID3D11DeviceContext1> DeviceContext() { return _device_context; }

    // Interface //
public:
    void Initialize(LUID adapter_luid, const std::vector<D3D_FEATURE_LEVEL> & feature_levels);

    void RenderXR(const XrRect2Di & image_rect,                                   //
                  const float render_target_clear_color[4],                       //
                  const std::vector<xr::math::ViewProjection> & view_projections, //
                  DXGI_FORMAT color_swapchain_format,                             //
                  ID3D11Texture2D * color_texture,                                //
                  DXGI_FORMAT depth_swapchain_format,                             //
                  ID3D11Texture2D * depth_texture,                                //
                  My::Engine::EntityManager & entity_manager,                     //
                  My::Engine::ComponentManager & component_manager);

    const std::vector<DXGI_FORMAT> & SupportedColorFormats() const
    {
        const static std::vector<DXGI_FORMAT> SupportedColorFormats = {
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
        };
        return SupportedColorFormats;
    }

    const std::vector<DXGI_FORMAT> & SupportedDepthFormats() const
    {
        const static std::vector<DXGI_FORMAT> SupportedDepthFormats = {
            DXGI_FORMAT_D32_FLOAT,
            DXGI_FORMAT_D16_UNORM,
            DXGI_FORMAT_D24_UNORM_S8_UINT,
            DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
        };
        return SupportedDepthFormats;
    }

private:
    void UpdateComponents(ComponentManager & manager);

    std::vector<VERTEX_DATA> MakeData(const MeshComponent & component);

    void CreateD3D11DeviceAndContext(IDXGIAdapter1 * adapter,
                                     const std::vector<D3D_FEATURE_LEVEL> & feature_levels,
                                     ID3D11Device ** device, ID3D11DeviceContext ** device_context);

    winrt::com_ptr<IDXGIAdapter1> GetAdapter(LUID adapter_id);

    void InitializeD3DResources();

    void InitializeShaders();

    void PreparePBRShader(const MaterialPointer & material_pointer, const XMVECTOR & camera_pos,
                          XMVECTOR * lights);

    void PreparePhongShader(const MaterialPointer & material_pointer, const XMVECTOR & camera_pos,
                            XMVECTOR * lights);
};

} // namespace _implementation

using _implementation::D3D11RenderSystem;

} // namespace My::D3D11RenderSystem