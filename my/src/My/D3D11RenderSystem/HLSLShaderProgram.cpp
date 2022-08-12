#include "pch.h"

#include "My/D3D11RenderSystem/HLSLShaderProgram.h"

void My::D3D11RenderSystem::_implementation::HLSLShaderProgram::AddShader(std::wstring filename,
                                                                          HLSLShaderType type)
{
    switch (type)
    {
        case HLSLShaderType::PixelShader:
            _pixel_code = LoadShaderFile(filename);
            winrt::check_hresult(
                _device->CreatePixelShader(static_cast<void *>(_pixel_code.data()), //
                                            _pixel_code.size(),                      //
                                            nullptr,                                  //
                                            _pixel_shader.put())                     //
            );
            break;
        case HLSLShaderType::GeometryShader:
            _geometry_code = LoadShaderFile(filename);
            winrt::check_hresult(
                _device->CreateGeometryShader(static_cast<void *>(_geometry_code.data()), //
                                              _geometry_code.size(),                      //
                                              nullptr,                                    //
                                              _geometry_shader.put())                     //
            );
            break;
        case HLSLShaderType::VertexShader:
            _vertex_code = LoadShaderFile(filename);
            winrt::check_hresult(
                _device->CreateVertexShader(static_cast<void *>(_vertex_code.data()), //
                                            _vertex_code.size(),                      //
                                            nullptr,                                  //
                                            _vertex_shader.put())                     //
            );
    }
}

void My::D3D11RenderSystem::_implementation::HLSLShaderProgram::Bind()
{
    _device_context->VSSetShader(_vertex_shader.get(), nullptr, 0);
    _device_context->GSSetShader(_geometry_shader.get(), nullptr, 0);
    _device_context->PSSetShader(_pixel_shader.get(), nullptr, 0);
}

winrt::com_ptr<ID3D11InputLayout>
My::D3D11RenderSystem::_implementation::HLSLShaderProgram::GenerateLayout(
    std::vector<D3D11_INPUT_ELEMENT_DESC> desc)
{
    winrt::check_hresult(_device->CreateInputLayout(desc.data(),                            //
                                                    static_cast<UINT>(desc.size()),         //
                                                    _vertex_code.data(),                    //
                                                    static_cast<UINT>(_vertex_code.size()), //
                                                    _input_layout.put()));
    return _input_layout;
}

std::vector<byte>
My::D3D11RenderSystem::_implementation::HLSLShaderProgram::LoadShaderFile(std::wstring filename)
{
    // open the file
    std::ifstream shader_file(filename, std::ios::in | std::ios::binary | std::ios::ate);

    std::vector<byte> data;
    assert(shader_file.is_open());

    int length = static_cast<int>(shader_file.tellg());

    data.resize(length);
    shader_file.seekg(0, std::ios::beg);
    shader_file.read(reinterpret_cast<char *>(data.data()), length);
    shader_file.close();

    return data;
}
