#include<Windows.h>
#include <memory>
import GameSystem.Execution;

int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hInstPrev, _In_ PSTR cmdline, _In_ int cmdshow)
{
    return std::make_unique<gameSystem::Execution>()->Run();
}
