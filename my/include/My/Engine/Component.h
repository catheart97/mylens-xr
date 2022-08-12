#pragma once

namespace My::Engine
{

namespace _implementation
{

struct Entity;
struct EntityReference
{
    size_t Index;
};

struct Component
{
    // Data Components //
public:
    EntityReference Parent;
    bool Active{false};
    // TODO: Local transform?

    // Interface Requirement //
public:
    virtual ~Component() = default;
};

} // namespace _implementation

using _implementation::Component;
using _implementation::EntityReference;

} // namespace My::Engine