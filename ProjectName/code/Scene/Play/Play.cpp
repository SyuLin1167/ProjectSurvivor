module;
#include <DxLib.h>
#include <thread>

module Scene.Play;

import MyLib.KeyStatus;
import Scene.Title;
import Scene.Result;
import Object.ObjectManager;
import Object.Player;
import Object.Enemy;
import Object.Map;
import Object.Goal;
import GameSystem.Camera;
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
            m_objManager->AddObject(new object::Map(true));
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
    }

    SceneCmd Play::Update()
    {
        // オブジェクト更新
        m_objManager->Update();

        if (m_prototype && input::KeyStatus::CheckKey(keyType.R, ON_PRESS))
            return CmdReplace{ []() { return std::make_shared<Title>(); } };

        // クリア後の画面へ移動
        if (m_cleared)
        {
            return CmdReplace{ [](){ return std::make_shared<Result>(); } };
        }

        // 継続
        return std::monostate{};
    }

    void Play::Draw()
    {
        if (!m_prototype) DrawScrollingBackground();
        m_objManager->Draw();
        if (m_prototype)
            DrawString(16, 16, "Phase 1 / WASD or arrows: move / R: return to title",
                GetColor(255, 255, 255));
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
