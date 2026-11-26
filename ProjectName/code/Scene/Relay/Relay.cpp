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
import <algorithm>;

namespace scene
{
    Relay::Relay(expedition::ExpeditionState state) : m_state(std::move(state))
    {
        // 遠征値だけを所有し、旧Play・敵・Colliderを保持しない。ボタンもシーンごとの新規個体。
        const float x = gameSystem::Window::GetWindowData()->SIZE.x / 2.0f - 450;
        auto name = m_ui.Create<ui::TextLabel>(quest::DisplayText(m_state.quest.name));
        m_ui.GetElement(name)->SetPosition({x, 300});

        // 選択肢の順序と個数は接続データが決める。ボタンへsafe/dangerの分岐を埋め込まない。
        float y = 480;
        for (const auto& id : m_state.CurrentNode().next)
        {
            const auto& node = expedition::DefaultMap().Find(id);
            ui::ButtonDef route(node.name, shape::Rect{{x, y}, {220, 50}}, GetColor(150,180,220));
            route.onReleased = [this, id] { SelectRoute(id); };
            m_ui.Create<ui::Button>(&route);
            auto detail = m_ui.Create<ui::TextLabel>(node.description);
            m_ui.GetElement(detail)->SetPosition({x + 240, y + 12});
            y += 100;
        }

        // 選択と出発を別の操作にし、選択クリックの解除で即時ロードしない。
        ui::ButtonDef next("選択ルートへ出発", shape::Rect{{x, 740}, {240, 50}}, GetColor(150,180,220));
        next.onReleased = [this] { m_continue = true; };
        m_depart = static_cast<ui::Button*>(m_ui.GetElement(m_ui.Create<ui::Button>(&next)));
        m_depart->SetEnabled(false);
        ui::ButtonDef title("Title", shape::Rect{{x + 270, 740}, {120, 50}}, GetColor(150,180,220));
        title.onReleased = [this] { m_title = true; };
        m_ui.Create<ui::Button>(&title);
    }

    bool Relay::SelectRoute(const std::string& id)
    {
        const auto& links = m_state.CurrentNode().next;
        // UI以外からの選択も同じ境界を通す。無効入力では既存の選択を変えない。
        if (m_leaving || std::find(links.begin(), links.end(), id) == links.end()) return false;
        m_selected = id;
        m_depart->SetEnabled(true);
        return true;
    }

    SceneCmd Relay::Continue()
    {
        // 未選択・連打ではロードしない。報酬は仮選択でなく、確定したコピーだけへ適用する。
        if (m_leaving || !m_selected) return std::monostate{};
        auto next = m_state;
        next.Advance(*m_selected);
        m_leaving = true;

        // LoadingSceneの背景用Relayは完了区間の値を保ち、新Playへは次ノードの値を渡す。
        return Play::Continue(shared_from_this(), std::move(next));
    }

    SceneCmd Relay::Update()
    {
        // 遷移確定後は入力を止め、別コマンドでロード要求を上書きしない。
        if (m_leaving) return std::monostate{};
        m_ui.Update(); // 中継中に戦闘・敵生成・成長の時計は進めない。

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
        const int x = int(gameSystem::Window::GetWindowData()->SIZE.x / 2) - 450;
        DrawFormatString(x, 360, GetColor(230,230,230), "RELAY  Section %d / %d complete",
            m_state.Section(), m_state.SectionCount());
        DrawFormatString(x, 410, GetColor(230,230,230), "HP %.0f / %.0f    Lv.%d",
            m_state.health, m_state.maxHealth, m_state.growth.Level());

        // 効果は次区間だけ有効。選択中も残HPを書き換えず、出発確定との違いを表示する。
        if (m_selected)
            DrawFormatString(x, 680, GetColor(230,230,230), "選択: %s / 効果は次区間から有効",
                expedition::DefaultMap().Find(*m_selected).name.c_str());
        else
            DrawString(x, 680, "ルートを選択してください", GetColor(230,230,230));
        m_ui.Draw();
    }
}