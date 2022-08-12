#pragma once

#include "pch.h"

#include "My/Engine/Component.h"
#include "My/Engine/MaterialStructures.h"

namespace My::Engine
{

namespace _implementation
{

struct MeshComponent : public Component
{
    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec3> Normals;
    std::vector<glm::vec2> UVs;

    std::vector<uint32_t> Indices;

    bool Wireframe{false};
    bool DoubleSided{false}; // Has no effect yet

    MaterialPointer MPointer;
};

} // namespace _implementation

using _implementation::MeshComponent;

} // namespace My::Engine