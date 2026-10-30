module;
#include <DxLib.h>
#include <thread>

module Scene.Play;

import MyLib.KeyStatus;
import Scene.Title;
import Scene.Result;
import Scene.LoadingScene;
import UI.PlayHud;
import Object.ObjectManager;
import Object.Player;
import Object.Enemy;
import Object.EnemyConfig;
import MyLib.File.FileSystem;
import Object.Map;
import Object.MovingBase;
import Object.Goal;
import GameSystem.Camera;
import GameSystem.FrameRate;
import Asset.Graph;
import AppContext;
import GameSystem.Window;
import <algorithm>;

using namespace gameSystem;

namespace scene
{
    Play::Play(bool prototype)
        : m_cleared(false)
        , m_prototype(prototype)
    {
        if (m_prototype)
        {
            m_objManager->AddObject(new object::Player(true));
            m_player = std::dynamic_pointer_cast<object::Player>(
                m_objManager->GetObjForTag(object::PLAYER).front());
            m_objManager->AddObject(new object::Map(true));
            auto* base = new object::MovingBase;
            m_objManager->AddObject(base);
            m_stage.emplace(base->StartX(), base->EndX());
            m_base = std::dynamic_pointer_cast<object::MovingBase>(
                m_objManager->GetObjForTag(object::BASE).front());
            // DEC-02: 前方・上下側方の固定3体。継続出現は行わない。
            auto& files = AppCtx::FileSystem();
            const auto enemies = phase2::EnemyConfig::FromJson(files.jsonIO.Load(files.GetDataDir() / "EnemyConfig"));
            for (const auto center : enemies.SpawnCenters)
                m_objManager->AddObject(new object::Enemy(center, enemies));
            return;
        }
        //オブジェクト追加
        m_objManager->AddObject(new object::Player);
        m_objManager->AddObject(new object::Enemy);
        m_objManager->AddObject(new object::Map);
        m_objManager->AddObject(new object::Goal(m_cleared));

        // 背景画像読み込み
        gameSystem::AppCtx::AssetMgr().LoadAsync<asset::Graph>("background", "Play.png");
    }

    Play::~Play()
    {
        if (m_prototype) return;
        // 背景画像ハンドル削除
        gameSystem::AppCtx::AssetMgr().DeleteHandle<asset::Graph>("background");
    }

    void Play::Init()
    {
        m_objManager->InitBeforeUpdate();
        // 全Object初期化後にカメラを合わせ、ロード時間をプレイへ持ち越さない。
        Camera::Instance().Update();
        FrameRate::ResetSimulationTime();
    }

    SceneCmd Play::Start(std::shared_ptr<SceneBase> hold, bool prototype)
    {
        return CmdReplace{ [hold = std::move(hold), prototype]() mutable {
            return std::make_shared<LoadingScene>(LoadPolicy::PROGRESS,
                [prototype]() { return std::make_shared<Play>(prototype); }, std::move(hold));
        } };
    }

    SceneCmd Play::Update()
    {
        // 結果確定後に再度Updateが呼ばれても、攻撃・移動・遷移を繰り返さない。
        if (m_stage && m_stage->Finished()) return std::monostate{};
        // オブジェクト更新
        m_objManager->Update();

        // Stepの接触被害とLateUpdateがすべて反映された後に判定する。
        if (m_stage)
        {
            const auto base = m_base.lock();
            if (!base) return CmdQuit{}; // 拠点消失は通常の勝敗に偽装しない。
            const auto end = m_stage->Advance(base->Center().x, base->Health(),
                input::KeyStatus::CheckKey(keyType.R, ON_PRESS));
            if (end == StageEnd::Success || end == StageEnd::Defeat)
                return CmdReplace{ [result = m_stage->ResultValue()]() {
                    return std::make_shared<Result>(result);
                } };
            if (end == StageEnd::Aborted)
                return CmdReplace{ []() { return std::make_shared<Title>(); } };
        }

        // クリア後の画面へ移動
        if (m_cleared)
        {
            return CmdReplace{ [](){ return std::make_shared<Result>(StageResult{StageEnd::Success, 100, 1}, false); } };
        }

        // 継続
        return std::monostate{};
    }

    void Play::Draw()
    {
        if (!m_prototype) DrawScrollingBackground();
        m_objManager->Draw();
        if (m_stage)
        {
            const auto base = m_base.lock();
            const auto player = m_player.lock();
            if (base && player)
            {
                // 初回描画も含め、そのPlayの実値を読む。共有・静的なHUD状態を残さない。
                const auto result = m_stage->ResultValue();
                const auto hud = ui::MakeHudValues(base->Health(), result.progress,
                    m_stage->Distance(), m_stage->RemainingDistance(), base->Center() - player->Center());
                ui::DrawPlayHud(hud, {Camera::Instance().WorldToScreen(base->Center() - base->Size().Half()), base->Size()});
            }
        }
    }

    void Play::DrawScrollingBackground(float parallax)
    {
        // カメラオフセット取得
        auto camOffset = gameSystem::Camera::Instance().GetOffset();

        // グラフアセット取得
        auto graph = gameSystem::AppCtx::AssetMgr().Fetch<asset::Graph>();
        if (!graph)
        {
            return;
        }

        // 背景画像ハンドル取得
        int bgHandle = graph->GetHandle("background");
        if (bgHandle < 0)
        {
            return;
        }

        // 背景サイズとウィンドウサイズ取得
        int bgW = 0, bgH = 0;
        GetGraphSize(bgHandle, &bgW, &bgH);
        auto winSize = Window::GetWindowData()->SIZE;

        // カメラオフセットに対して背景のパララックス移動量
        int scrollX = static_cast<int>(-camOffset.x * parallax);
        int drawY = static_cast<int>(-camOffset.y * parallax);

        // Yはそのまま描画（必要なら同様にタイル化可能）
        int drawX1 = scrollX % bgW;
        drawX1 = (drawX1 + bgW < 0) ? drawX1 += bgW : drawX1;
        drawX1 = (drawX1 > bgW) ? drawX1 -= bgW : drawX1;

        int drawX2 = scrollX % bgW + bgW;
        drawX2 = (drawX2 + bgW < 0) ? drawX2 += bgW : drawX2;
        drawX2 = (drawX2 > bgW) ? drawX2 -= bgW : drawX2;

        // 2枚で水平ループ
        DrawGraph(drawX1, drawY, bgHandle, true);
        DrawGraph(drawX2, drawY, bgHandle, true);

        // 画面幅を覆うために追加描画
        if (winSize.x > bgW)
        {
            DrawGraph(drawX2 + bgW, drawY, bgHandle, true);
        }
    }
}
