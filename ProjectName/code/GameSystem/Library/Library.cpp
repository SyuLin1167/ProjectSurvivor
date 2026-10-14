module;
#include <Dxlib.h>

module GameSystem.Library;

namespace gameSystem
{
    Library::Library()
        :m_success(true)
    {
        SetWindowSizeExtendRate(1.0);
        SetMainWindowText("2DGame");

        SetUseDirect3DVersion(DX_DIRECT3D_11);

        if (DxLib_Init())
        {
            DebugBreak();
            m_success = false;
        }

        SetDrawScreen(DX_SCREEN_BACK);
    }

    Library::~Library()
    {
        DxLib_End();
    }
}
