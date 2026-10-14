module;
#include <DxLib.h>

module GameSystem.Window;

namespace gameSystem
{
    Window::Window()
    {
        m_winInfo.reset(new Info);

        ChangeWindowMode(true);
        SetGraphMode(m_winInfo->SIZE.x, m_winInfo->SIZE.y, m_winInfo->COLOR_BIT);
    }
}