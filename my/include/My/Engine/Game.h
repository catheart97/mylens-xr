#pragma once

#include "pch.h"

namespace My::Engine
{

class Game
{
public:
    virtual ~Game() = default;
    virtual void Run() = 0;
};

std::unique_ptr<Game> CreateGame();

} // namespace My::Engine