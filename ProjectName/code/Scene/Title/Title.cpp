module;
#include <DxLib.h>

module Scene.Title;

import MyLib.Math.Vector2;
import MyLib.Shape.Rect;
import MyLib.KeyStatus;
import MyLib.Loading.LoadingContext;
import Scene.Play;
import Asset.Graph;
import UI.Button;

using namespace gameSystem;
using namespace math;

namespace scene
{
    Title::Title()
    {
        // 背景画像読み込み
        AppCtx::AssetMgr().LoadAsync<asset::Graph>("Title", "Title.png");

        // 出発前だけ依頼を選べる。各ボタンのコールバックは依頼の定義を値で保持する。
        float y = 680;
        for (const auto& entry : quest::AvailableQuests)
        {
            ui::ButtonDef choice(entry.name, shape::Rect{{680, y}, {560, 48}}, GetColor(160, 178, 190));
            choice.onReleased = [this, entry]() { m_selectedQuest = entry; };
            m_questButtons.push_back(AppCtx::UIMgr().Create<ui::Button>(&choice));
            y += 54;
        }

        // スタートボタン作成
        shape::Rect buttonRect(START_BTN_POS, BTN_SIZE);
        ui::ButtonDef startButtonDef("Start", buttonRect, GetColor(150, 100,80));
        startButtonDef.onReleased = [this]() { m_toNextScene = true; };
        m_startButtonID = AppCtx::UIMgr().Create<ui::Button>(&startButtonDef);

        // Exitボタン作成
        buttonRect.pos = EXIT_BTN_POS;
        ui::ButtonDef exitButtonDef("Exit", buttonRect, GetColor(150, 100, 80));
        exitButtonDef.onReleased = [this]() { m_toExit = true; };
        m_exitButtonID = AppCtx::UIMgr().Create<ui::Button>(&exitButtonDef);
    }

    Title::~Title()
    {
        // 背景画像ハンドル削除
        gameSystem::AppCtx::AssetMgr().DeleteHandle<asset::Graph>("Title");

        for (auto id : m_questButtons) AppCtx::UIMgr().Destroy(id);
        // ボタン破棄
        AppCtx::UIMgr().Destroy(m_startButtonID);
        AppCtx::UIMgr().Destroy(m_exitButtonID);
    }

    SceneCmd Title::Update()
    {
        AppCtx::UIMgr().Update();

        // 必須画像の読み込み完了後に未登録なら、ログを残して安全に終了
        if (!task::LoadingContext::Get() || !task::LoadingContext::Get()->IsLoading())
        {
            if (AppCtx::AssetMgr().Fetch<asset::Graph>()->GetHandle("Title") == -1)
            {
                ErrorLogFmtAdd("Title required resource missing: Title.png");
                return CmdQuit{};
            }
        }
        // Startボタンが押されたらシーン移動
        if (m_toNextScene)
        {
            // Retryと同じ経路で毎回新規Playを生成する。
            return Play::Start(shared_from_this(), m_selectedQuest.id);
        }

        // Exitボタンが押されていたら終了要求
        if (m_toExit)
        {
            return CmdQuit{};
        }

        // 継続
        return std::monostate{};
    }

    void Title::Draw()
    {
        // タイトル画面描画
        DrawGraph(0, 0, AppCtx::AssetMgr().Fetch<asset::Graph>()->GetHandle("Title"), true);

        // 選択中の依頼と効果を同じ背景板に表示し、Start前に確認できるようにする。
        const auto& entry = m_selectedQuest;
        DrawBox(660, 790, 1260, 875, GetColor(28, 35, 45), TRUE);
        DrawFormatString(680, 802, GetColor(255, 238, 188), "選択中：%s　初期・最大HP %+d", entry.name, entry.healthBonus);
        DrawString(680, 838, entry.description, GetColor(255, 255, 255));
        // UI描画
        AppCtx::UIMgr().Draw();
    }

    void Title::DrawLoading()
    {
        // ローディング進捗度表示
        float progress = task::LoadingContext::Get()->NowProgress() * 100.0f;
        DrawFormatString(PROGRESS_TEXT_POS.x, PROGRESS_TEXT_POS.y, GetColor(100, 250, 150), "ローディング進捗度：%.2f％", progress);
        DrawBox(PROGRESS_TEXT_POS.x, PROGRESS_TEXT_POS.y - 50, PROGRESS_TEXT_POS.x + static_cast<int>(progress * 2), PROGRESS_TEXT_POS.y - 20, GetColor(0, 255, 0), true);
    }
}
