#pragma once

#include "My/Engine/Entity.h"

namespace My::Engine
{

class EntityManager
{
private:
    std::vector<Entity> _entities;

public:
    EntityReference CreateEntity()
    {
        _entities.push_back(Entity{});
        return EntityReference{_entities.size() - 1};
    }

    auto begin() { return _entities.begin(); }

    auto end() { return _entities.end(); }

    Entity & operator[](size_t index) { return _entities[index]; }
};

} // namespace My::Engine