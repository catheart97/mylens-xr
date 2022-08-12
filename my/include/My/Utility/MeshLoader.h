#pragma once

#include "pch.h"

#include "My/Utility/WavefrontLoader.h"

namespace My::Utility
{

namespace _implementation
{

class MeshLoader
{
public:
    static EntityReference Load(std::wstring filename, EntityManager & entity_manager,
                                ComponentManager & component_manager)
    {
        if (filename.ends_with(L".obj"))
        {
            return WavefrontLoader::Load(filename, entity_manager, component_manager);
        }
        else
        {
            throw std::runtime_error("File format not supported.");
        }
    }
};

} // namespace _implementation

using _implementation::MeshLoader;

} // namespace My::Utility