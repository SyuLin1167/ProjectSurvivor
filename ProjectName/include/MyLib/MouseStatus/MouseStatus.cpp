module;
#include <DxLib.h>

module MyLib.MouseStatus;

namespace input
{
    MouseStatus::MouseStatus()
    {
        GetMousePoint((int*)&mousePos.x, (int*)&mousePos.y);

        // 使用するクリックボタンの初期化
        const int buttons[] =
        {
            MOUSE_INPUT_LEFT,
            MOUSE_INPUT_RIGHT,
            MOUSE_INPUT_MIDDLE
        };

        for (int button : buttons)
        {
            clickData.emplace(button, CLICK_ON_RELEASE);
        }
    }

    void MouseStatus::UpdateMouseState()
    {
        GetMousePoint(&Instance().mousePos.x, &Instance().mousePos.y);
        Instance().UpdateClickStateImpl();
    }

    void MouseStatus::UpdateClickStateImpl()
    {
        int mouseState = GetMouseInput();

        //キーのステータスを一通り切り替える
        for (auto& click : clickData)
        {
            // 押下中かどうかで場合分け
            if (mouseState & click.first)
            {
                // 押下中の場合
                if (click.second & (CLICK_ON_RELEASE | CLICK_RELEASING))
                {
                    click.second = CLICK_ON_PRESS;
                    continue;
                }
                click.second = CLICK_PRESSING;
            }
            else
            {
                // 押下されていない場合
                if (click.second & (CLICK_ON_PRESS | CLICK_PRESSING))
                {
                    click.second = CLICK_ON_RELEASE;
                    continue;
                }
                click.second = CLICK_RELEASING;
            }
        }
    }
}
