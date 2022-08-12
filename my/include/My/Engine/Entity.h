#pragma once

#include "pch.h"

#include "My/Engine/Component.h"

namespace My::Engine
{

namespace _implementation
{

enum class ComponentType
{
    // NEW COMPONENT --> HERE
    MeshComponent,
    PointLightComponent
};

struct ComponentReference
{
    const size_t Index;
    const ComponentType Type;
};

struct Entity
{
    glm::vec3 Position{0.f, 0.f, 0.f};
    glm::quat Rotation{1.f, 0.f, 0.f, 0.f};
    glm::vec3 Scale{1, 1, 1};

    std::vector<ComponentReference> Components;
};

} // namespace _implementation

using _implementation::ComponentReference;
using _implementation::ComponentType;
using _implementation::Entity;

} // namespace My::Engine