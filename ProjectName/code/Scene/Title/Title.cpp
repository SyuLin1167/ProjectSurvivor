module;
#include <DxLib.h>

module Scene.Title;

import MyLib.Math.Vector2;
import MyLib.Shape.Rect;
import MyLib.KeyStatus;
import MyLib.Loading.LoadingContext;
import Scene.QuestSelect;
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

        // スタートボタン作成
        shape::Rect buttonRect(START_BTN_POS, BTN_SIZE);
        ui::ButtonDef startButtonDef("Start", buttonRect, GetColor(150, 100,80));
        startButtonDef.onReleased = [this]() { m_toNextScene = true; };
        m_ui.Create<ui::Button>(&startButtonDef);

        // Exitボタン作成
        buttonRect.pos = EXIT_BTN_POS;
        ui::ButtonDef exitButtonDef("Exit", buttonRect, GetColor(150, 100, 80));
        exitButtonDef.onReleased = [this]() { m_toExit = true; };
        m_ui.Create<ui::Button>(&exitButtonDef);
    }

    Title::~Title()
    {
        // 背景画像ハンドル削除
        gameSystem::AppCtx::AssetMgr().DeleteHandle<asset::Graph>("Title");

        // ボタンはシーン内UIManagerの破棄で解放する。
    }

    SceneCmd Title::Update()
    {
        m_ui.Update();

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
            // 依頼選択へ進み、出発時に共通Loading経路を使う。
            return CmdReplace{ [] { return std::make_shared<QuestSelect>(); } };
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

        // UI描画
        m_ui.Draw();
    }

    void Title::DrawLoading()
    {
        // ローディング進捗度表示
        float progress = task::LoadingContext::Get()->NowProgress() * 100.0f;
        DrawFormatString(PROGRESS_TEXT_POS.x, PROGRESS_TEXT_POS.y, GetColor(100, 250, 150), "ローディング進捗度：%.2f％", progress);
        DrawBox(PROGRESS_TEXT_POS.x, PROGRESS_TEXT_POS.y - 50, PROGRESS_TEXT_POS.x + static_cast<int>(progress * 2), PROGRESS_TEXT_POS.y - 20, GetColor(0, 255, 0), true);
    }
}
