module;
#include <DxLib.h>
module Scene.Relay;
import Scene.Play;
import Scene.Title;
import Quest.QuestText;
import UI.Button;
import UI.DisplayElements;
import MyLib.Shape.Rect;
import GameSystem.Window;

namespace scene
{
    Relay::Relay(expedition::ExpeditionState state) : m_state(std::move(state))
    {
        // 旧Playは保持しない。中継中はUI以外の更新対象を持たない。
        const float x = gameSystem::Window::GetWindowData()->SIZE.x / 2.0f - 170;
        auto name = m_ui.Create<ui::TextLabel>(quest::DisplayText(m_state.quest.name));
        m_ui.GetElement(name)->SetPosition({x, 320});
        ui::ButtonDef next("Next section", shape::Rect{{x, 540}, {200, 50}}, GetColor(150,180,220));
        next.onReleased = [this] { m_continue = true; };
        m_ui.Create<ui::Button>(&next);
        ui::ButtonDef title("Title", shape::Rect{{x + 220, 540}, {120, 50}}, GetColor(150,180,220));
        title.onReleased = [this] { m_title = true; };
        m_ui.Create<ui::Button>(&title);
    }

    SceneCmd Relay::Continue()
    {
        if (m_leaving) return std::monostate{};
        auto next = m_state;
        next.Advance();
        m_leaving = true;
        return Play::Continue(shared_from_this(), std::move(next));
    }

    SceneCmd Relay::Update()
    {
        if (m_leaving) return std::monostate{};
        m_ui.Update();
        if (m_continue) return Continue();
        if (m_title)
        {
            m_leaving = true;
            return CmdReplace{ [] { return std::make_shared<Title>(); } };
        }
        return std::monostate{};
    }

    void Relay::Draw()
    {
        const int x = int(gameSystem::Window::GetWindowData()->SIZE.x / 2) - 170;
        DrawFormatString(x, 380, GetColor(230,230,230), "RELAY  Section %d / %d complete", m_state.section, m_state.SectionCount);
        DrawFormatString(x, 430, GetColor(230,230,230), "HP %.0f / %.0f    Lv.%d", m_state.health, m_state.maxHealth, m_state.growth.Level());
        DrawString(x, 480, "Choose Next section when ready", GetColor(230,230,230));
        m_ui.Draw();
    }
}
