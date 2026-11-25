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
        // 遠征値だけを所有し、旧Playや戦闘オブジェクトは保持しない。
        // 依頼名と操作ボタンは、この中継シーン専用のUIとして画面中央付近に配置する。
        const float x = gameSystem::Window::GetWindowData()->SIZE.x / 2.0f - 170;
        auto name = m_ui.Create<ui::TextLabel>(quest::DisplayText(m_state.quest.name));
        m_ui.GetElement(name)->SetPosition({x, 320});

        // UIの走査中には遷移せず、要求フラグだけ立ててUpdateで処理する。
        ui::ButtonDef next("Next section", shape::Rect{{x, 540}, {200, 50}}, GetColor(150,180,220));
        next.onReleased = [this] { m_continue = true; };
        m_ui.Create<ui::Button>(&next);

        // 遠征を続けず終了する操作も、同じUI更新後の遷移経路へ集約する。
        ui::ButtonDef title("Title", shape::Rect{{x + 220, 540}, {120, 50}}, GetColor(150,180,220));
        title.onReleased = [this] { m_title = true; };
        m_ui.Create<ui::Button>(&title);
    }

    SceneCmd Relay::Continue()
    {
        // 遷移を要求済みなら何もせず、連打による二重ロードを防ぐ。
        if (m_leaving) return std::monostate{};

        // ロード中の中継表示を保つため、完了区間の値は変更せずコピーを進める。
        // Advanceは区間番号を進め、引き継ぐHP・成長状態はそのまま残す。
        auto next = m_state;
        next.Advance();
        m_leaving = true;

        // LoadingSceneの背景としてRelayを保持し、新しいPlayには遠征値を渡す。
        return Play::Continue(shared_from_this(), std::move(next));
    }

    SceneCmd Relay::Update()
    {
        // 遷移確定後はUI入力も止め、別の遷移要求で上書きしない。
        if (m_leaving) return std::monostate{};

        // 中継中はUIだけ更新する。戦闘・敵生成・ゲーム時間は進めない。
        m_ui.Update();

        // 同じ更新で両方要求された場合は、次区間への出発を優先する。
        if (m_continue) return Continue();
        if (m_title)
        {
            // Titleへ遠征値を渡さず、Relayの解放とともに今回の状態を破棄する。
            m_leaving = true;
            return CmdReplace{ [] { return std::make_shared<Title>(); } };
        }

        // 明示的な操作がなければ中継地点に留まり、自動では出発しない。
        return std::monostate{};
    }

    void Relay::Draw()
    {
        // 旧区間のカメラ位置に影響されない画面座標で、完了時点の状態を表示する。
        const int x = int(gameSystem::Window::GetWindowData()->SIZE.x / 2) - 170;
        DrawFormatString(x, 380, GetColor(230,230,230), "RELAY  Section %d / %d complete", m_state.section, m_state.SectionCount);
        DrawFormatString(x, 430, GetColor(230,230,230), "HP %.0f / %.0f    Lv.%d", m_state.health, m_state.maxHealth, m_state.growth.Level());

        // 操作待ちであることを案内し、依頼名とボタンを重ねて描画する。
        DrawString(x, 480, "Choose Next section when ready", GetColor(230,230,230));
        m_ui.Draw();
    }
}
