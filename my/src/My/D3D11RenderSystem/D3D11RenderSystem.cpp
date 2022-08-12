#include "pch.h"

#include "My/D3D11RenderSystem/D3D11RenderSystem.h"

void My::D3D11RenderSystem::_implementation::D3D11RenderSystem::Initialize(
    LUID adapter_luid, const std::vector<D3D_FEATURE_LEVEL> & feature_levels)
{
    const winrt::com_ptr<IDXGIAdapter1> adapter = GetAdapter(adapter_luid);

    winrt::com_ptr<ID3D11Device> device{nullptr};
    winrt::com_ptr<ID3D11DeviceContext> device_context{nullptr};
    CreateD3D11DeviceAndContext(adapter.get(), feature_levels, device.put(), device_context.put());

    _device = device.as<ID3D11Device1>();
    _device_context = device_context.as<ID3D11DeviceContext1>();

    InitializeD3DResources();
}

void My::D3D11RenderSystem::_implementation::D3D11RenderSystem::UpdateComponents(
    ComponentManager & manager)
{
    const auto & mesh_components = manager.GetMeshComponentData();
    for (size_t i = 0; i < mesh_components.size(); ++i)
    {
        auto & component = mesh_components[i];
        if (i >= _vertex_buffers.size()) // TODO: Dirty Flag
        {
            // vertex data buffer
            D3D11_BUFFER_DESC v_buffer_desc{0};
            v_buffer_desc.ByteWidth =
                static_cast<UINT>(sizeof(VERTEX_DATA) * component.Vertices.size());
            v_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

            auto data = MakeData(component);
            D3D11_SUBRESOURCE_DATA v_sub_data{data.data(), 0, 0};

            winrt::com_ptr<ID3D11Buffer> vertex_buffer{nullptr};
            winrt::check_hresult(
                _device->CreateBuffer(&v_buffer_desc, &v_sub_data, vertex_buffer.put()));

            // index data buffer
            D3D11_BUFFER_DESC i_buffer_desc{0};
            i_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE; // TODO: Maybe flag?
            i_buffer_desc.ByteWidth = static_cast<UINT>(sizeof(UINT) * component.Indices.size());
            i_buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

            winrt::com_ptr<ID3D11Buffer> index_buffer{nullptr};
            D3D11_SUBRESOURCE_DATA i_sub_data{component.Indices.data(), 0, 0};
            winrt::check_hresult(
                _device->CreateBuffer(&i_buffer_desc, &i_sub_data, index_buffer.put()));

            _vertex_buffers.push_back(vertex_buffer);
            _index_buffers.push_back(index_buffer);
        }
    }
}

std::vector<My::D3D11RenderSystem::VERTEX_DATA>
My::D3D11RenderSystem::_implementation::D3D11RenderSystem::MakeData(const MeshComponent & component)
{
    std::vector<VERTEX_DATA> data(component.Vertices.size());
    for (size_t i = 0; i < component.Vertices.size(); ++i)
    {
        bool fill = component.UVs.size() != component.Vertices.size();
        data[i] = {cv(component.Vertices[i]), //
                   cv(component.Normals[i]),  //
                   fill ? XMFLOAT2{0, 0} : cv(component.UVs[i])};
    }
    return data;
}

void My::D3D11RenderSystem::_implementation::D3D11RenderSystem::CreateD3D11DeviceAndContext(
    IDXGIAdapter1 * adapter, const std::vector<D3D_FEATURE_LEVEL> & feature_levels,
    ID3D11Device ** device, ID3D11DeviceContext ** device_context)
{
    UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
    creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // Create the Direct3D 11 API device object and a corresponding context.
    D3D_DRIVER_TYPE driver_type =
        adapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN;

    while (1)
    {
        const HRESULT hr = D3D11CreateDevice(adapter, driver_type, 0, creation_flags,
                                             feature_levels.data(), (UINT)feature_levels.size(),
                                             D3D11_SDK_VERSION, device, nullptr, device_context);

        if (FAILED(hr))
        {
            if ((creation_flags & D3D11_CREATE_DEVICE_DEBUG) &&
                (hr == DXGI_ERROR_SDK_COMPONENT_MISSING))
            {
                creation_flags &= ~D3D11_CREATE_DEVICE_DEBUG;
                continue;
            }

            if (driver_type != D3D_DRIVER_TYPE_WARP)
            {
                driver_type = D3D_DRIVER_TYPE_WARP;
                continue;
            }
        }
        else
            break;
    }
}

winrt::com_ptr<IDXGIAdapter1>
My::D3D11RenderSystem::_implementation::D3D11RenderSystem::GetAdapter(LUID adapter_id)
{
    // Create the DXGI factory.
    winrt::com_ptr<IDXGIFactory1> dxgi_factory;
    CHECK_HRCMD(CreateDXGIFactory1(winrt::guid_of<IDXGIFactory1>(), dxgi_factory.put_void()));

    for (UINT adapter_index = 0;; adapter_index++)
    {
        // EnumAdapters1 will fail with DXGI_ERROR_NOT_FOUND when there are no more adapters
        // to enumerate.
        winrt::com_ptr<IDXGIAdapter1> dxgi_adapter;
        CHECK_HRCMD(dxgi_factory->EnumAdapters1(adapter_index, dxgi_adapter.put()));

        DXGI_ADAPTER_DESC1 adapter_desc;
        CHECK_HRCMD(dxgi_adapter->GetDesc1(&adapter_desc));
        if (memcmp(&adapter_desc.AdapterLuid, &adapter_id, sizeof(adapter_id)) == 0)
        {
            DEBUG_PRINT("Using graphics adapter %ws", adapter_desc.Description);
            return dxgi_adapter;
        }
    }
}

void My::D3D11RenderSystem::_implementation::D3D11RenderSystem::InitializeD3DResources()
{
    // Constant Vertex Buffer //
    CONSTANT_VS constant_all; // Intitial value;
    constant_all.ModelMatrix = XMMatrixIdentity();
    constant_all.ViewMatrix[0] = XMMatrixIdentity();
    constant_all.ViewMatrix[1] = XMMatrixIdentity();
    constant_all.ProjectionMatrix[0] = XMMatrixIdentity();
    constant_all.ProjectionMatrix[1] = XMMatrixIdentity();
    constant_all.NormalMatrix = XMMatrixIdentity();

    D3D11_BUFFER_DESC constant_all_desc;
    constant_all_desc.ByteWidth = sizeof(CONSTANT_VS);
    constant_all_desc.Usage = D3D11_USAGE_DYNAMIC;
    constant_all_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constant_all_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constant_all_desc.MiscFlags = 0;
    constant_all_desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA constant_all_subres_data;
    constant_all_subres_data.pSysMem = &constant_all;
    constant_all_subres_data.SysMemPitch = 0;
    constant_all_subres_data.SysMemSlicePitch = 0;

    winrt::check_hresult(
        _device->CreateBuffer(&constant_all_desc, &constant_all_subres_data, &_constant_vs_buffer));

    _device_context->VSSetConstantBuffers(0, 1, &_constant_vs_buffer);

    D3D11_RASTERIZER_DESC1 rasterizer_state_desc;
    rasterizer_state_desc.FillMode = D3D11_FILL_SOLID;
    rasterizer_state_desc.CullMode = D3D11_CULL_BACK;    //
    rasterizer_state_desc.FrontCounterClockwise = false; // true
    rasterizer_state_desc.DepthBias = 0;
    rasterizer_state_desc.SlopeScaledDepthBias = 0;
    rasterizer_state_desc.DepthBiasClamp = 0;
    rasterizer_state_desc.DepthClipEnable = true;
    rasterizer_state_desc.ScissorEnable = false;
    rasterizer_state_desc.MultisampleEnable = true; // TODO: Multisampling
    rasterizer_state_desc.AntialiasedLineEnable = true;
    rasterizer_state_desc.ForcedSampleCount = 0;

    // TODO: Double Sided Rasterizers
    winrt::check_hresult(
        _device->CreateRasterizerState1(&rasterizer_state_desc, _rasterizer_state_solid.put()));

    rasterizer_state_desc.FillMode = D3D11_FILL_WIREFRAME;
    rasterizer_state_desc.CullMode = D3D11_CULL_NONE;
    winrt::check_hresult(
        _device->CreateRasterizerState1(&rasterizer_state_desc, _rasterizer_state_wf.put()));

    _device_context->RSSetState(_rasterizer_state_solid.get());
    Utility::winrtUtility::LogMessage(L"Rasterizer(s) initialized.");

    Utility::winrtUtility::LogMessage(L"Direct3D 11 initialized.");

    // Alpha Blending // TODO:
    ID3D11BlendState1 * blend_state{nullptr};
    D3D11_BLEND_DESC1 blend_state_desc;
    ZeroMemory(&blend_state_desc, sizeof(D3D11_BLEND_DESC1));
    blend_state_desc.RenderTarget[0].BlendEnable = true;
    blend_state_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_state_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_state_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_state_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_state_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blend_state_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_state_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    winrt::check_hresult(_device->CreateBlendState1(&blend_state_desc, &blend_state));

    _device_context->OMSetBlendState(blend_state, 0, 0xffffffff);

    D3D11_FEATURE_DATA_D3D11_OPTIONS3 options;
    _device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS3, &options, sizeof(options));
    CHECK_MSG(options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizer,
              "This sample requires VPRT support. Adjust sample shaders on GPU without VRPT.");

    CD3D11_DEPTH_STENCIL_DESC depth_stencil_desc(CD3D11_DEFAULT{});
    depth_stencil_desc.DepthEnable = true;
    depth_stencil_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc = D3D11_COMPARISON_GREATER;
    CHECK_HRCMD(_device->CreateDepthStencilState(&depth_stencil_desc,
                                                 _reversed_zdepth_no_stencil_test.put()));

    InitializeShaders();
}

void My::D3D11RenderSystem::_implementation::D3D11RenderSystem::InitializeShaders()
{
    // Throwbridge Reitz GGX

    _pbr_shader = std::make_unique<HLSLShaderProgram>(_device, _device_context);
    _pbr_shader->AddShader(L"PBRVertexShader.cso", HLSLShaderType::VertexShader);
    _pbr_shader->AddShader(L"PBRPixelShader.cso", HLSLShaderType::PixelShader);

    _pbr_input_layout = _pbr_shader->GenerateLayout(DEFAULT_SHADER_LAYOUT_DESC);

    CONSTANT_PS_PBR pbr_constant;
    D3D11_BUFFER_DESC pbr_constant_desc;
    pbr_constant_desc.ByteWidth = sizeof(CONSTANT_PS_PBR);
    pbr_constant_desc.Usage = D3D11_USAGE_DYNAMIC;
    pbr_constant_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    pbr_constant_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    pbr_constant_desc.MiscFlags = 0;
    pbr_constant_desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA pbr_constant_all_subres_data;
    pbr_constant_all_subres_data.pSysMem = &pbr_constant;
    pbr_constant_all_subres_data.SysMemPitch = 0;
    pbr_constant_all_subres_data.SysMemSlicePitch = 0;

    winrt::check_hresult(_device->CreateBuffer(&pbr_constant_desc, &pbr_constant_all_subres_data,
                                               &_pbr_constant_buffer));

    // Phong Model //

    _phong_shader = std::make_unique<HLSLShaderProgram>(_device, _device_context);
    _phong_shader->AddShader(L"PhongVertexShader.cso", HLSLShaderType::VertexShader);
    _phong_shader->AddShader(L"PhongPixelShader.cso", HLSLShaderType::PixelShader);

    CONSTANT_PS_PHONG phong_constant;
    D3D11_BUFFER_DESC phong_constant_desc;
    phong_constant_desc.ByteWidth = sizeof(CONSTANT_PS_PHONG);
    phong_constant_desc.Usage = D3D11_USAGE_DYNAMIC;
    phong_constant_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    phong_constant_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    phong_constant_desc.MiscFlags = 0;
    phong_constant_desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA phong_constant_all_subres_data;
    phong_constant_all_subres_data.pSysMem = &phong_constant;
    phong_constant_all_subres_data.SysMemPitch = 0;
    phong_constant_all_subres_data.SysMemSlicePitch = 0;

    winrt::check_hresult(_device->CreateBuffer(
        &phong_constant_desc, &phong_constant_all_subres_data, &_phong_constant_buffer));
}

void My::D3D11RenderSystem::_implementation::D3D11RenderSystem::PreparePBRShader(
    const MaterialPointer & material_pointer, const XMVECTOR & camera_pos, XMVECTOR * lights)
{
    _pbr_shader->Bind();
    _device_context->PSSetConstantBuffers(0, 1, &_pbr_constant_buffer);

    const PBRMaterialData & data = material_pointer.AsPBRMaterial();

    CONSTANT_PS_PBR pbr_constant;
    pbr_constant.albedo = data.Albedo;
    pbr_constant.alpha = data.Alpha;
    pbr_constant.ambient_occlusion = data.AmbientOcclusion;
    pbr_constant.ior = data.IOR;
    pbr_constant.metalness = data.Metalness;
    pbr_constant.roughness = data.Roughness;
    pbr_constant.normal_map = 0.0f;

    XMStoreFloat3(&pbr_constant.camera, camera_pos);
    for (size_t i = 0; i < MAX_NUMBER_LIGHTS; ++i) pbr_constant.lights[i] = lights[i];

    D3D11_MAPPED_SUBRESOURCE subres{0};
    winrt::check_hresult(
        _device_context->Map(_pbr_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres));
    memcpy(subres.pData, &pbr_constant, sizeof(CONSTANT_PS_PBR));
    _device_context->Unmap(_pbr_constant_buffer, 0);

    _device_context->IASetInputLayout(_pbr_input_layout.get());
}

void My::D3D11RenderSystem::_implementation::D3D11RenderSystem::PreparePhongShader(
    const MaterialPointer & material_pointer, const XMVECTOR & camera_pos, XMVECTOR * lights)
{
    _phong_shader->Bind();
    _device_context->PSSetConstantBuffers(0, 1, &_phong_constant_buffer);

    const PhongMaterialData & data = material_pointer.AsPhongMaterial();

    CONSTANT_PS_PHONG phong_constant;

    phong_constant.ambient = data.Ambient;
    phong_constant.alpha = data.Alpha;
    phong_constant.shininess = data.Shininess;
    phong_constant.specular = data.Specular;
    phong_constant.diffuse = data.Diffuse;

    XMStoreFloat3(&phong_constant.camera, camera_pos);
    for (size_t i = 0; i < MAX_NUMBER_LIGHTS; ++i) phong_constant.lights[i] = lights[i];

    D3D11_MAPPED_SUBRESOURCE subres{0};
    winrt::check_hresult(
        _device_context->Map(_phong_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres));
    memcpy(subres.pData, &phong_constant, sizeof(CONSTANT_PS_PHONG));
    _device_context->Unmap(_phong_constant_buffer, 0);

    _device_context->IASetInputLayout(_phong_input_layout.get());
}

void My::D3D11RenderSystem::_implementation::D3D11RenderSystem::RenderXR(
    const XrRect2Di & image_rect, const float render_target_clear_color[4],
    const std::vector<xr::math::ViewProjection> & view_projections,
    DXGI_FORMAT color_swapchain_format, ID3D11Texture2D * color_texture,
    DXGI_FORMAT depth_swapchain_format, ID3D11Texture2D * depth_texture,
    My::Engine::EntityManager & entity_manager, My::Engine::ComponentManager & component_manager)
{
    const uint32_t view_instance_count = (uint32_t)view_projections.size();
    CHECK_MSG(view_instance_count == 2, "Shader supports 2 view instances.")

    CD3D11_VIEWPORT viewport((float)image_rect.offset.x, (float)image_rect.offset.y,
                             (float)image_rect.extent.width, (float)image_rect.extent.height);
    _device_context->RSSetViewports(1, &viewport);

    winrt::com_ptr<ID3D11RenderTargetView> render_target_view;
    const CD3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc(D3D11_RTV_DIMENSION_TEXTURE2DARRAY,
                                                                 color_swapchain_format);
    CHECK_HRCMD(_device->CreateRenderTargetView(color_texture, &render_target_view_desc,
                                                render_target_view.put()));

    winrt::com_ptr<ID3D11DepthStencilView> depth_stencil_view;
    CD3D11_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc(D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
                                                           depth_swapchain_format);
    CHECK_HRCMD(_device->CreateDepthStencilView(depth_texture, &depth_stencil_view_desc,
                                                depth_stencil_view.put()));

    const bool reversed_z = view_projections[0].NearFar.Near > view_projections[0].NearFar.Far;
    const float depthClearValue = reversed_z ? 0.f : 1.f;

    _device_context->ClearRenderTargetView(render_target_view.get(), render_target_clear_color);
    _device_context->ClearDepthStencilView(
        depth_stencil_view.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depthClearValue, 0);
    _device_context->OMSetDepthStencilState(
        reversed_z ? _reversed_zdepth_no_stencil_test.get() : nullptr, 0);

    ID3D11RenderTargetView * render_targets[] = {render_target_view.get()};
    _device_context->OMSetRenderTargets((UINT)std::size(render_targets), render_targets,
                                        depth_stencil_view.get());

    const DirectX::XMMATRIX l_view = xr::math::LoadInvertedXrPose(view_projections[0].Pose);
    const DirectX::XMMATRIX r_view = xr::math::LoadInvertedXrPose(view_projections[1].Pose);
    const DirectX::XMMATRIX l_proj =
        ComposeProjectionMatrix(view_projections[0].Fov, view_projections[0].NearFar);
    const DirectX::XMMATRIX r_proj =
        ComposeProjectionMatrix(view_projections[1].Fov, view_projections[1].NearFar);

    CONSTANT_VS constant_vs;
    constant_vs.ModelMatrix = XMMatrixIdentity();
    constant_vs.ViewMatrix[0] = l_view;
    constant_vs.ViewMatrix[1] = r_view;
    constant_vs.ProjectionMatrix[0] = l_proj;
    constant_vs.ProjectionMatrix[1] = r_proj;
    constant_vs.NormalMatrix = XMMatrixIdentity();

    auto point_light_components = component_manager.GetPointLightComponentData();

    XMVECTOR lights[MAX_NUMBER_LIGHTS];

    size_t i = 0;

    for (; i < std::min<size_t>(MAX_NUMBER_LIGHTS, point_light_components.size()); ++i)
    {
        lights[i] =
            XMVectorSetByIndex(cvx(entity_manager[point_light_components[i].Parent.Index].Position),
                               point_light_components[i].Intensity, 3);
    }

    for (; i < MAX_NUMBER_LIGHTS; ++i) lights[i] = XMVectorSet(0, 0, 0, 0);

    auto & eye_position = view_projections[0].Pose.position;

    UpdateComponents(component_manager);

    auto mesh_components = component_manager.GetMeshComponentData();
    for (size_t i = 0; i < mesh_components.size(); ++i)
    {
        const MeshComponent & mesh_component = mesh_components[i];
        auto & material_pointer = mesh_component.MPointer;

        _device_context->RSSetState(mesh_component.Wireframe ? _rasterizer_state_wf.get()
                                                             : _rasterizer_state_solid.get());

        auto & entity = entity_manager[mesh_component.Parent.Index];
        constant_vs.ModelMatrix = XMMatrixAffineTransformation(
            cvx(entity.Scale), XMVectorSet(0, 0, 0, 0), cvx(entity.Rotation), cvx(entity.Position));

        XMVECTOR det{0};
        constant_vs.NormalMatrix =
            XMMATRIX(XMMatrixTranspose(XMMatrixInverse(&det, constant_vs.ModelMatrix)));

        D3D11_MAPPED_SUBRESOURCE subres{0};
        winrt::check_hresult(
            _device_context->Map(_constant_vs_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &subres));
        memcpy(subres.pData, &constant_vs, sizeof(CONSTANT_VS));
        _device_context->Unmap(_constant_vs_buffer, 0);

        switch (material_pointer.Type())
        {
            case MaterialType::PBRMaterial:
                PreparePBRShader(material_pointer,                                               //
                                 XMVectorSet(eye_position.x, eye_position.y, eye_position.z, 0), //
                                 lights);
                break;
            case MaterialType::PhongMaterial:
                PreparePhongShader(
                    material_pointer,                                               //
                    XMVectorSet(eye_position.x, eye_position.y, eye_position.z, 0), //
                    lights);
                break;
        }

        UINT stride{sizeof(VERTEX_DATA)}, offset{0};
        auto * v_buffer = _vertex_buffers[i].get(); // simulate array
        _device_context->IASetVertexBuffers(0, 1, &v_buffer, &stride, &offset);
        _device_context->IASetIndexBuffer(_index_buffers[i].get(), DXGI_FORMAT_R32_UINT, 0);

        _device_context->IASetPrimitiveTopology(
            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // TODO: Flag
        _device_context->DrawIndexedInstanced(
            static_cast<UINT>(mesh_component.Vertices.size()), // Index count per instance.
            view_instance_count,                               // Instance count.
            0,                                                 // Start index Location.
            0,                                                 // Base vertex Location.
            0                                                  // Start Instance Location
        );
    }
}
