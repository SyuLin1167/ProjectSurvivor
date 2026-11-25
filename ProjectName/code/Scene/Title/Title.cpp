module;
#include <DxLib.h>
module Scene.Title;
import MyLib.Shape.Rect;
import MyLib.Loading.LoadingContext;
import Scene.QuestSelect;
import Quest.QuestCatalog;
import Asset.Graph;

using namespace gameSystem;

namespace scene
{
    Title::Title()
    {
        AppCtx::AssetMgr().LoadAsync<asset::Graph>("Title", "Title.png");
        auto& record = AppCtx::Campaign();
        // 遠征から戻っただけでは、未保存の攻略記録を古いファイルで巻き戻さない。
        if (record.Status() == campaign::SaveStatus::Unloaded) record.Load(quest::KnownIds());

        ui::ButtonDef resume("続きから", shape::Rect{{840, 740}, {240, 50}}, GetColor(150, 180, 220));
        resume.onReleased = [this] { m_action = Action::Continue; };
        m_continueButton = static_cast<ui::Button*>(m_ui.GetElement(m_ui.Create<ui::Button>(&resume)));
        ui::ButtonDef fresh("最初から", shape::Rect{{840, 810}, {240, 50}}, GetColor(150, 180, 220));
        fresh.onReleased = [this] { m_action = Action::New; };
        m_newButton = static_cast<ui::Button*>(m_ui.GetElement(m_ui.Create<ui::Button>(&fresh)));
        ui::ButtonDef exit("終了", shape::Rect{{840, 880}, {240, 50}}, GetColor(150, 180, 220));
        exit.onReleased = [this] { m_action = Action::Exit; };
        m_exitButton = static_cast<ui::Button*>(m_ui.GetElement(m_ui.Create<ui::Button>(&exit)));

        // 確認は別UIで所有する。通常ボタンと同じ入力フレームに反応させない。
        ui::ButtonDef accept("初期化して開始", shape::Rect{{730, 570}, {220, 50}}, GetColor(220, 140, 120));
        accept.onReleased = [this] { m_action = Action::Accept; };
        m_acceptButton = static_cast<ui::Button*>(m_confirmation.GetElement(m_confirmation.Create<ui::Button>(&accept)));
        ui::ButtonDef cancel("キャンセル", shape::Rect{{990, 570}, {200, 50}}, GetColor(150, 180, 220));
        cancel.onReleased = [this] { m_action = Action::Cancel; };
        m_cancelButton = static_cast<ui::Button*>(m_confirmation.GetElement(m_confirmation.Create<ui::Button>(&cancel)));
        RefreshButtons();
    }

    Title::~Title() { AppCtx::AssetMgr().DeleteHandle<asset::Graph>("Title"); }

    void Title::RefreshButtons()
    {
        // 確認を開いた時点で通常ボタンの押下履歴を捨て、取消後の入力連鎖を防ぐ。
        m_continueButton->SetEnabled(!m_confirming && AppCtx::Campaign().Ready());
        m_newButton->SetEnabled(!m_confirming);
        m_exitButton->SetEnabled(!m_confirming);
        m_acceptButton->SetEnabled(m_confirming);
        m_cancelButton->SetEnabled(m_confirming);
    }

    SceneCmd Title::ContinueGame()
    {
        auto& record = AppCtx::Campaign();
        record.Load(quest::KnownIds());
        RefreshButtons();
        if (!record.Ready() || !record.Save()) return std::monostate{};
        return CmdReplace{ [] { return std::make_shared<QuestSelect>(); } };
    }

    SceneCmd Title::RequestNewGame()
    {
        auto& record = AppCtx::Campaign();
        // 起動後に保存ファイルが追加・変更された場合も、無確認で上書きしない。
        record.Load(quest::KnownIds());
        if (record.Status() == campaign::SaveStatus::Missing) return StartNew();
        m_confirming = true;
        RefreshButtons();
        return std::monostate{};
    }

    SceneCmd Title::ConfirmNewGame(bool accepted)
    {
        if (!m_confirming) return std::monostate{};
        m_confirming = false;
        RefreshButtons();
        if (accepted) return StartNew();
        return std::monostate{};
    }

    SceneCmd Title::StartNew()
    {
        if (!AppCtx::Campaign().StartNew()) return std::monostate{};
        return CmdReplace{ [] { return std::make_shared<QuestSelect>(); } };
    }

    SceneCmd Title::Update()
    {
        if (m_confirming) m_confirmation.Update();
        else m_ui.Update();
        if (!task::LoadingContext::Get() || !task::LoadingContext::Get()->IsLoading())
        {
            if (AppCtx::AssetMgr().Fetch<asset::Graph>()->GetHandle("Title") == -1)
            {
                ErrorLogFmtAdd("Title required resource missing: Title.png");
                return CmdQuit{};
            }
        }
        // UIのコールバックではファイルを変更せず、走査後に要求を一度だけ処理する。
        const auto action = m_action;
        m_action = Action::None;
        switch (action)
        {
        case Action::Continue: return ContinueGame();
        case Action::New: return RequestNewGame();
        case Action::Accept: return ConfirmNewGame(true);
        case Action::Cancel: return ConfirmNewGame(false);
        case Action::Exit: return CmdQuit{};
        default: return std::monostate{};
        }
    }

    void Title::Draw()
    {
        DrawGraph(0, 0, AppCtx::AssetMgr().Fetch<asset::Graph>()->GetHandle("Title"), true);
        DrawBox(500, 660, 1420, 960, GetColor(28, 35, 45), TRUE);
        DrawString(530, 680, AppCtx::Campaign().Message().c_str(), GetColor(240, 225, 180));
        m_ui.Draw();
        if (m_confirming)
        {
            DrawBox(620, 420, 1300, 650, GetColor(40, 45, 55), TRUE);
            DrawString(660, 460, "現在の攻略記録を初期化しますか？", GetColor(255, 230, 190));
            DrawString(660, 500, "破損・非対応の記録も置換されます。取消では変更しません。", GetColor(255, 255, 255));
            m_confirmation.Draw();
        }
    }
}
