#pragma once

#include "pch.h"

#include "My/Color.h"

namespace My::Engine
{

using namespace My;

namespace _implementation
{

/*
 * The whole process cannot be realised using inheritance as this violates the 16-byte alignment
 * restriction in Direct3D
 */

enum class MaterialType
{
    // NEW SHADER --> HERE
    PBRMaterial = 0,
    PhongMaterial = 1
};


struct PBRMaterialData
{
    Color Albedo;
    float Alpha;
    float Roughness;
    float IOR;
    float AmbientOcclusion;
    float Metalness;
};

struct PhongMaterialData
{
    Color Ambient;
    float Alpha;
    float Shininess;
    Color Diffuse;
    Color Specular;
};

#define REGISTER_MATERIAL(mat_data, mat_type)                                                      \
    MaterialPointer(mat_data data) : _mtype{MaterialType::mat_type}                                \
    {                                                                                              \
        auto data_ = std::make_shared<mat_data>(data);                                             \
        _data = std::reinterpret_pointer_cast<void>(data_);                                        \
    }                                                                                              \
    mat_data As##mat_type() const                                                                  \
    {                                                                                              \
        if (Is##mat_type())                                                                        \
        {                                                                                          \
            return *(std::reinterpret_pointer_cast<mat_data>(_data).get());                        \
        }                                                                                          \
        throw std::runtime_error("Incorrect Material type cannot call As##");                      \
    }                                                                                              \
    bool Is##mat_type() const { return MaterialType::mat_type == _mtype; }

class MaterialPointer
{
private:
    std::shared_ptr<void> _data{nullptr};
    MaterialType _mtype;

public:
    MaterialPointer() {}

    // NEW SHADER --> HERE
    REGISTER_MATERIAL(PBRMaterialData, PBRMaterial)
    REGISTER_MATERIAL(PhongMaterialData, PhongMaterial)

    MaterialType Type() const { return _mtype; }
};
} // namespace _implementation

using _implementation::MaterialPointer;
using _implementation::MaterialType;
using _implementation::PBRMaterialData;
using _implementation::PhongMaterialData;

} // namespace My::Engine