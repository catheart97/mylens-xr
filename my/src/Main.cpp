#include "pch.h"

#include "My/Engine/Game.h"

constexpr const char * ProgramName = "my";

int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    try
    {
        auto game = My::Engine::CreateGame();
        game->Run();
    }
    catch (const std::exception & ex)
    {
        DEBUG_PRINT("Unhandled Exception: %s\n", ex.what());
        return 1;
    }
    catch (...)
    {
        DEBUG_PRINT("Unhandled Exception\n");
        return 1;
    }
    return 0;
}
