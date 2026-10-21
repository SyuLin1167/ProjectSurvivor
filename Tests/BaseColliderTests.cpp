#include <cmath>
#include <cstdlib>
#include <iostream>

import Collider.ColliderManager;
import Collider;
import ObjectTag;
import Object.BaseMotion;
import GameSystem.Camera;
import GameSystem.CameraGeometry;
import MyLib.Math.Vector2;

void require(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

int main()
{
    // 実際のRectCollider/Stepを使い、停止・遠方・重なり・追跡で基盤の移動反映を測る。
    // ObjectManager、実キー入力、描画、シーン遷移の確認は別途必要。
    for (int fps : {30, 60, 120})
    {
        for (int scenario = 0; scenario < 3; ++scenario)
        {
            col2d::ColliderManager manager;
            col2d::ColliderDef baseDef{}, playerDef{};
            baseDef.localPos = {720, 2016}; // 中心(768,2048)、96x64
            playerDef.localPos = scenario == 0 ? math::Vector2f{496, 2032} :
                scenario == 1 ? math::Vector2f{16, 16} : math::Vector2f{752, 2032};
            const auto base = manager.CreateRectCollider(&baseDef, {96, 64}, object::BASE);
            const auto player = manager.CreateRectCollider(&playerDef, {32, 32}, object::PLAYER);
            auto& camera = gameSystem::Camera::Instance();
            math::Vector2f playerCenter{};
            camera.SetWorldSize({16384, 4096});
            camera.SetTarget(&playerCenter, true);
            int offscreenFrames = 0;
            for (int i = 0; i < fps * 60; ++i)
            {
                auto* b = manager.GetCollider(base);
                const float center = b->GetColliderDef()->localPos.x + 48;
                b->SetVelocity(phase1::BaseDisplacement(center, 15000, 80, 1.0f / fps));
                // マスク未登録なので重なっても押し戻さない。Pの有無や変位でBを制御しない。
                manager.GetCollider(player)->SetVelocity({scenario == 2 ? 240.0f / fps : 0, 0});
                manager.Step();
                playerCenter = manager.GetCollider(player)->GetColliderDef()->localPos + math::Vector2f{16, 16};
                camera.Update();
                const auto baseCenter = b->GetColliderDef()->localPos + math::Vector2f{48, 32};
                if (cameraGeometry::BaseMarker(camera.WorldToScreen(baseCenter), {48, 32}, {1920, 1080}).offscreen)
                    ++offscreenFrames;
            }
            const auto position = manager.GetCollider(base)->GetColliderDef()->localPos;
            const float error = std::abs((position.x - 720) - 4800);
            std::cout << "Collider " << fps << "fps scenario=" << scenario
                      << " distance=" << position.x - 720 << " error=" << error << "u\n";
            require(error < 2 && position.y == 2016, "Independent collider movement within 2u");
            // 実Cameraで画面外になったフレームを含めても、拠点の60秒距離は同じ。
            require(offscreenFrames > 0, "Base keeps moving outside actual camera viewport");
            camera.ClearTarget(&playerCenter);
            // 破棄後に残ったコライダーを動かせることも、このマスクなし構成で確認する。
            manager.Destroy(player);
            manager.GetCollider(base)->SetVelocity({1, 0});
            manager.Step();
            require(manager.GetCollider(base)->GetColliderDef()->localPos.x == position.x + 1,
                "Continue after player collider destruction");
        }
    }
    std::cout << "PASS: real ColliderManager::Step, stationary/distant/overlap player, destruction\n";
}
