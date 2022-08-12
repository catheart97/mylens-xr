#pragma once

#include "My/Engine/Component.h"

namespace My::Engine
{

struct PointLightComponent : public Component
{
    float Intensity;
};

} // namespace My::Engine