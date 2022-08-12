#pragma once

#include "pch.h"

namespace My::D3D11RenderSystem
{

namespace _implementation
{

enum class HLSLShaderType
{
    PixelShader,
    GeometryShader,
    VertexShader
};

class HLSLShaderProgram
{
private:
    winrt::com_ptr<ID3D11Device1> _device{nullptr};
    winrt::com_ptr<ID3D11DeviceContext1> _device_context{nullptr};

    winrt::com_ptr<ID3D11VertexShader> _vertex_shader{nullptr};
    winrt::com_ptr<ID3D11GeometryShader> _geometry_shader{nullptr};
    winrt::com_ptr<ID3D11PixelShader> _pixel_shader{nullptr};

    std::vector<byte> _vertex_code;
    std::vector<byte> _geometry_code;
    std::vector<byte> _pixel_code;

    winrt::com_ptr<ID3D11InputLayout> _input_layout;

public:
    HLSLShaderProgram(winrt::com_ptr<ID3D11Device1> device,
                      winrt::com_ptr<ID3D11DeviceContext1> device_context)
        : _device{device}, _device_context{device_context}
    {}

    void AddShader(std::wstring filename, HLSLShaderType type);

    void Bind();

    winrt::com_ptr<ID3D11InputLayout> GenerateLayout(std::vector<D3D11_INPUT_ELEMENT_DESC> desc);

protected:
    static std::vector<byte> LoadShaderFile(std::wstring filename);
};

} // namespace _implementation

using _implementation::HLSLShaderProgram;
using _implementation::HLSLShaderType;

} // namespace My::D3D11RenderSystem