#pragma once

#include "My/Engine/ComponentManager.h"
#include "My/Engine/EntityManager.h"

namespace My::Engine
{

class System
{
public:
    virtual ~System() = default;
};

} // namespace My::Engine