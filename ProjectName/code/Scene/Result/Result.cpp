module;
#include <DxLib.h>
#include <cmath>

module Scene.Result;

import Scene.Title;
import Scene.Play;
import AppContext;
import UI.Button;
import MyLib.Shape.Rect;
import GameSystem.Window;
import UI.PlayHud;

namespace scene
{
    Result::Result(StageResult result, bool retryPrototype)
        : m_result(result), m_retryPrototype(retryPrototype)
    {
        const float centerX = gameSystem::Window::GetWindowData()->SIZE.x / 2.0f;
        ui::ButtonDef retry("Retry", shape::Rect{{centerX - 160, 600}, {140, 50}}, GetColor(150, 180, 220));
        retry.onReleased = [this]() { m_toRetry = true; };
        m_retryButtonID = gameSystem::AppCtx::UIMgr().Create<ui::Button>(&retry);
        ui::ButtonDef title("Title", shape::Rect{{centerX + 20, 600}, {140, 50}}, GetColor(150, 180, 220));
        title.onReleased = [this]() { m_toTitle = true; };
        m_titleButtonID = gameSystem::AppCtx::UIMgr().Create<ui::Button>(&title);
        // 新しいButtonは押下履歴を持たず、前画面からの解放だけでは作動しない。
        // Rのショートカットを設けず、結果確定フレームの入力連鎖も避ける。
    }

    Result::~Result()
    {
        gameSystem::AppCtx::UIMgr().Destroy(m_retryButtonID);
        gameSystem::AppCtx::UIMgr().Destroy(m_titleButtonID);
    }

    SceneCmd Result::Update()
    {
        if (m_leaving) return std::monostate{};
        gameSystem::AppCtx::UIMgr().Update();
        if (m_toRetry)
        {
            m_leaving = true;
            return Play::Start(shared_from_this(), m_retryPrototype);
        }
        if (m_toTitle)
        {
            m_leaving = true;
            return CmdReplace{ []() { return std::make_shared<Title>(); } };
        }
        return std::monostate{};
    }

    void Result::Draw()
    {
        const int x = gameSystem::Window::GetWindowData()->SIZE.x / 2 - 160;
        const auto color = GetColor(255, 255, 255);
        DrawString(x, 400, m_result.outcome == StageEnd::Success ? "SUCCESS" : "DEFEAT", color);
        // 生きたObjectを読み直さず、確定値をPlayと共通の表示へ渡す。
        ui::DrawStageStatus(m_result.health, m_result.progress, x, 450);
        DrawString(x, 550, "Click Retry or Title", color);
        gameSystem::AppCtx::UIMgr().Draw();
    }
}
