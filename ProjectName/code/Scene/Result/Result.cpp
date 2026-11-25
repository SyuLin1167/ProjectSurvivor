module;
#include <DxLib.h>
#include <cmath>

module Scene.Result;

import Scene.Title;
import Quest.QuestText;
import Scene.Play;
import AppContext;
import UI.Button;
import MyLib.Shape.Rect;
import GameSystem.Window;
import UI.PlayHud;
import UI.DisplayElements;

namespace scene
{
    Result::Result(StageResult result, expedition::ExpeditionState state)
        : m_expedition(std::move(state)), m_result(result)
    {
        const float centerX = gameSystem::Window::GetWindowData()->SIZE.x / 2.0f;
        const float x = centerX - 160;
        const auto name = m_hudManager.Create<ui::TextLabel>(quest::DisplayText(m_expedition.quest.name));
        m_hudManager.GetElement(name)->SetPosition({x, 350});
        const auto heading = m_hudManager.Create<ui::TextLabel>(m_result.outcome == StageEnd::Success ? "EXPEDITION SUCCESS" : "EXPEDITION DEFEAT");
        m_hudManager.GetElement(heading)->SetPosition({x, 400});
        const auto status = m_hudManager.Create<ui::StageStatus>(m_result.health, m_result.progress, m_expedition.maxHealth);
        m_hudManager.GetElement(status)->SetPosition({x, 450});
        const auto controls = m_hudManager.Create<ui::TextLabel>("Click Retry or Title");
        m_hudManager.GetElement(controls)->SetPosition({x, 550});
        ui::ButtonDef retry("Retry", shape::Rect{{centerX - 160, 600}, {140, 50}}, GetColor(150, 180, 220));
        retry.onReleased = [this]() { m_toRetry = true; };
        m_hudManager.Create<ui::Button>(&retry);
        ui::ButtonDef title("Title", shape::Rect{{centerX + 20, 600}, {140, 50}}, GetColor(150, 180, 220));
        title.onReleased = [this]() { m_toTitle = true; };
        m_hudManager.Create<ui::Button>(&title);
        // 保存に失敗しても成功結果を失わせず、この画面から再試行できるようにする。
        if (gameSystem::AppCtx::Campaign().Dirty())
        {
            ui::ButtonDef save("保存を再試行", shape::Rect{{centerX - 160, 720}, {220, 50}}, GetColor(220, 180, 140));
            save.onReleased = [this] { m_saveRequested = true; };
            m_hudManager.Create<ui::Button>(&save);
        }
        // 新しいButtonは押下履歴を持たず、前画面からの解放だけでは作動しない。
        // Rのショートカットを設けず、結果確定フレームの入力連鎖も避ける。
    }


    SceneCmd Result::Update()
    {
        if (m_leaving) return std::monostate{};
        m_hudManager.Update();
        if (m_saveRequested)
        {
            m_saveRequested = false;
            gameSystem::AppCtx::Campaign().Save();
        }
        if (m_toRetry)
        {
            m_leaving = true;
            return Retry();
        }
        if (m_toTitle)
        {
            m_leaving = true;
            return CmdReplace{ []() { return std::make_shared<Title>(); } };
        }
        return std::monostate{};
    }

    SceneCmd Result::Retry()
    {
        return Play::Start(shared_from_this(), m_expedition.quest.id);
    }

    void Result::Draw()
    {
        m_hudManager.Draw();
        const int x = int(gameSystem::Window::GetWindowData()->SIZE.x / 2) - 160;
        DrawFormatString(x, 525, GetColor(230,230,230), "Section %d / %d    Lv.%d",
            m_expedition.section, m_expedition.SectionCount, m_expedition.growth.Level());
        if (gameSystem::AppCtx::Campaign().Dirty())
            DrawString(500, 680, "未保存：終了すると今回の攻略記録が失われます。保存を再試行してください。", GetColor(255, 170, 140));
        else
            DrawString(700, 680, "攻略記録は保存済みです。", GetColor(200, 230, 200));
    }
}
