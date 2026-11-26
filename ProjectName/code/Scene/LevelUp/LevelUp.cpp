module;
#include <DxLib.h>
module Scene.LevelUp;
import UI.Button;
import GameSystem.Window;
import MyLib.Shape.Rect;
import <array>;
namespace scene
{
    LevelUp::LevelUp(progression::ProgressionState& growth) : m_growth(growth)
    {
        const std::array<const char*,4> labels{"攻撃力 +20%", "攻撃速度 +15%", "移動速度 +10%", "磁石範囲 +25%"};
        const auto size = gameSystem::Window::GetWindowData()->SIZE;
        float y = float(size.y) / 2 - 50;
        for (const auto choice : growth.Choices())
        {
            ui::ButtonDef button(labels[static_cast<size_t>(choice)],
                shape::Rect{{float(size.x)/2 - 200, y}, {400, 60}}, GetColor(170,200,220));
            button.onReleased = [this, choice] { m_choice = choice; };
            m_ui.Create<ui::Button>(&button);
            y += 80;
        }
    }
    SceneCmd LevelUp::Update()
    {
        m_ui.Update();
        if (m_choice && m_growth.Select(*m_choice)) return CmdPop{};
        return std::monostate{};
    }
    void LevelUp::Draw()
    {
        const auto size = gameSystem::Window::GetWindowData()->SIZE;
        SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
        DrawBox(0,0,size.x,size.y,GetColor(0,0,0),TRUE);
        SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
        DrawString(size.x/2-200,size.y/2-110,"LEVEL UP / 強化を1つ選択",GetColor(255,255,255));
        // 大量取得時も残り選択数を示し、同じ画面の繰り返しを停止と誤認させない。
        DrawFormatString(size.x/2-200, size.y/2-85, GetColor(255,255,255),
            "Lv.%d / %d   残り選択: %d", m_growth.Level(), progression::MaxLevel, m_growth.Pending());
        m_ui.Draw();
    }
}
