#include <Windows.h>
#include <crtdbg.h>
#include <cstdio>
#include <cstring>
#include <memory>
import GameSystem.Execution;
import Scene.Play;

// Diagnostic entry point: no mouse/keyboard input is simulated.
// A queued quit request exercises ProcessMessage's termination path.
int main(int argc, char** argv)
{
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, 0);
    _CrtMemState before{}, alive{}, after{};
    _CrtMemCheckpoint(&before);
    int result;
    {
        auto execution = std::make_unique<gameSystem::Execution>();
        if (argc > 1 && std::strcmp(argv[1], "--play") == 0)
        {
            for (int i = 0; i < 10; ++i)
            {
                scene::Play play(true);
                play.Init();
            }
        }
        _CrtMemCheckpoint(&alive);
        PostQuitMessage(0);
        result = execution->Run();
    }
    _CrtMemCheckpoint(&after);
    std::printf("Run=%d normal blocks before/alive/after=%zu/%zu/%zu bytes=%zu/%zu/%zu\n",
        result, before.lCounts[_NORMAL_BLOCK], alive.lCounts[_NORMAL_BLOCK],
        after.lCounts[_NORMAL_BLOCK], before.lSizes[_NORMAL_BLOCK],
        alive.lSizes[_NORMAL_BLOCK], after.lSizes[_NORMAL_BLOCK]);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    std::fprintf(stderr, "After Execution destruction:\n");
    const int remaining = _CrtDumpMemoryLeaks();
    std::printf("Post-destruction CRT=%d\n", remaining);
    if (argc > 1 && std::strcmp(argv[1], "--leak") == 0)
    {
        // Positive control only: ensure the final CRT diagnostic still sees leaks.
        volatile void* leaked = _malloc_dbg(37, _NORMAL_BLOCK, __FILE__, __LINE__);
        if (!leaked) return 2;
    }
    std::fprintf(stderr, "Automatic CRT report after static destruction follows:\n");
    return result;
}
