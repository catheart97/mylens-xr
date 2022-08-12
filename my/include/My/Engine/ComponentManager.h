#pragma once

#include "pch.h"

#include "My/Engine/Component.h"
#include "My/Engine/Entity.h"
#include "My/Engine/EntityManager.h"
#include "My/Engine/LightComponents.h"
#include "My/Engine/MeshComponent.h"

#include "My/Utility/Macros.h"

namespace My::Engine
{

namespace _implementation
{

#define REGISTER_COMPONENT(comp_t)                                                                 \
private:                                                                                           \
    std::vector<comp_t> COMBINE(_##comp_t, Data);                                                  \
                                                                                                   \
public:                                                                                            \
    ComponentReference RegisterComponent(EntityManager & eman, EntityReference e_ref,              \
                                         comp_t component)                                         \
    {                                                                                              \
        component.Parent = e_ref;                                                                  \
        COMBINE(_##comp_t, Data).push_back(component);                                             \
        ComponentReference ref{COMBINE(_##comp_t, Data).size() - 1, ComponentType::comp_t};        \
        eman[e_ref.Index].Components.push_back(ref);                                               \
        return ref;                                                                                \
    }                                                                                              \
    const std::vector<comp_t> & COMBINE(Get##comp_t, Data)() const                                 \
    {                                                                                              \
        return COMBINE(_##comp_t, Data);                                                           \
    }

class ComponentManager
{
    // NEW COMPONENT --> HERE
    REGISTER_COMPONENT(MeshComponent)
    REGISTER_COMPONENT(PointLightComponent)
};

} // namespace _implementation

using _implementation::ComponentManager;

} // namespace My::Engine