#include <cmath>
#include <cstdlib>
#include <iostream>
#include <filesystem>

import Collider.ColliderManager;
import Collider;
import ObjectTag;
import Object.BaseMotion;
import GameSystem.Camera;
import GameSystem.CameraGeometry;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;
import Object.Enemy;
import Object.Player;
import Object.PlayerAttackConfig;
import <algorithm>;
import Object.EnemyConfig;
import MyLib.File.FileSystem;
import Object.MovingBase;
import Object.ObjectManager;
import GameSystem.FrameRate;
import AppContext;

/// <summary>回帰条件に失敗した場合は理由を表示してテストを終了する。</summary>
/// <param name="condition">成立を期待する検証条件。</param>
/// <param name="message">条件不成立時に表示する理由。</param>
void require(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

/// <summary>同種個体の削除で他個体のカテゴリ登録を失わないことを確認する。</summary>
void VerifyCategoryLifetime()
{
    col2d::ColliderManager manager;
    col2d::ColliderDef def{};
    const auto base = manager.CreateRectCollider(&def, {96, 64}, object::BASE);
    const auto first = manager.CreateCircleCollider(&def, 16, object::ENEMY);
    const auto second = manager.CreateCircleCollider(&def, 16, object::ENEMY);
    const auto key = col2d::MakeKey(col2d::CIRCLE, object::ENEMY);
    int notifications = 0;
    manager.AddMask(base, col2d::CIRCLE, object::ENEMY);
    col2d::ContactListener listener;
    listener.event = [&]() { ++notifications; };
    manager.AddEvent(base, key, listener);
    manager.Destroy(first);
    require(manager.GetCollider(first) == nullptr && manager.GetCollider(second) != nullptr, "Individual ID destruction");
    require(manager.GetCollider(base)->GetFilter().HasMask(key), "Category mask survives sibling destruction");
    manager.GetCollider(base)->TriggerEvent(key, {});
    require(notifications == 1, "Category event survives sibling destruction");
    manager.Destroy(second);
    const auto replacement = manager.CreateCircleCollider(&def, 16, object::ENEMY);
    require(manager.GetCollider(first) == nullptr && manager.GetCollider(second) == nullptr, "Stale generation stays invalid");
    manager.GetCollider(base)->TriggerEvent(key, {});
    require(notifications == 2 && manager.GetCollider(replacement), "Category registration survives respawn");
}

/// <summary>実Enemy/ObjectManagerで追跡、対象消失、シーン単位の破棄を検証する。</summary>
void VerifyEnemyPursuit()
{
    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    std::filesystem::create_directories(files.GetDataDir());
    std::filesystem::copy_file("ProjectName/resources/data/Phase1Config.json",
        files.GetDataDir() / "Phase1Config.json", std::filesystem::copy_options::overwrite_existing);
    // テスト中だけ実体は非constの時間値を固定し、本番用の時間注入APIを増やさない。
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const float previousDt = dt;
    const auto source = files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json");
    const auto initial = phase2::EnemyConfig::FromJson(source);
    require(initial.Health == 30 && initial.Radius == 16 && initial.Speed == 120 &&
        initial.SpawnCenters.size() == 3, "Committed JSON matches approved defaults");
    auto invalid = source;
    invalid["speed"] = 141;
    bool rejected = false;
    try { phase2::EnemyConfig::FromJson(invalid); }
    catch (const std::exception&) { rejected = true; }
    require(rejected, "JSON rejects speed outside delegated range");
    for (int scenario = 0; scenario < 4; ++scenario)
    {
        // 最後の60fpsではメモリ上のJSONだけを変更し、速度と配置の反映を実Enemyで測る。
        auto data = source;
        const bool changed = scenario == 3;
        const int fps = scenario == 0 ? 30 : scenario == 2 ? 120 : 60;
        if (changed) { data["speed"] = 140; data["spawnCenters"][0]["x"] = 1344; }
        const auto config = phase2::EnemyConfig::FromJson(data);
        dt = 1.0f / fps;
        bool finished = false;
        object::ObjectManager objects;
        auto* base = new object::MovingBase(finished);
        objects.AddObject(base);
        for (auto position : config.SpawnCenters)
            objects.AddObject(new object::Enemy(position, config));
        objects.InitBeforeUpdate();
        auto enemies = objects.GetObjForTag(object::ENEMY);
        std::weak_ptr<object::GameObject> removed = enemies.front();
        const auto start = enemies.front()->AccessPos().NowPos();
        require(start == config.SpawnCenters.front(), "JSON spawn center reaches actual Enemy");
        for (int frame = 0; frame < fps; ++frame) objects.Update();
        const auto end = enemies.front()->AccessPos().NowPos();
        const float displacement = (end - start).Length();
        require(std::abs(displacement - config.Speed) < 0.1f, "Enemy speed uses elapsed seconds");
        require(std::abs(base->Center().x - 848) < 0.1f, "Enemy does not obstruct caravan");
        std::cout << "Enemy " << fps << "fps configured=" << config.Speed << " first-second displacement=" << displacement << "u\n";
        for (int frame = 0; frame < fps * 9; ++frame) objects.Update();
        for (const auto& enemy : enemies)
            require((enemy->AccessPos().NowPos() - base->Center()).Length() < 5, "Three enemies converge on moving caravan");
        objects.RemoveObject(enemies.front().get());
        enemies.erase(enemies.begin());
        require(removed.expired(), "Enemy removed without retained ownership");
        // 参照先だけを消して、弱参照が失効した後も残りの敵を更新できることを確認する。
        objects.RemoveObject(base);
        const auto stopped = enemies.front()->AccessPos().NowPos();
        for (int frame = 0; frame < fps; ++frame) objects.Update();
        require((enemies.front()->AccessPos().NowPos() - stopped).Length() == 0, "Missing target stops pursuit");
        enemies.clear(); // Managerより前にテスト側の共有参照を解放する。
    }
    dt = previousDt;
    gameSystem::AppCtx::Deactivate();
    std::cout << "PASS: actual Enemy/ObjectManager pursuit and target destruction\n";
}

/// <summary>実Player/Enemyで自動攻撃の選択、間隔、撃破、再試行を検証する。</summary>
void VerifyAutoAttack()
{
    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    for (const auto name : {"Phase1Config.json", "PlayerAttackConfig.json"})
        std::filesystem::copy_file(std::filesystem::path("ProjectName/resources/data") / name,
            files.GetDataDir() / name, std::filesystem::copy_options::overwrite_existing);
    const auto config = phase2::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const auto previousDt = dt;
    auto attackData = files.jsonIO.Load("ProjectName/resources/data/PlayerAttackConfig.json");
    const auto attack = phase2::PlayerAttackConfig::FromJson(attackData);
    require(attack.Range == 160 && attack.Damage == 10 && attack.Interval == 0.5f, "Approved attack defaults");
    attackData["damage"] = 11;
    bool rejected = false;
    try { phase2::PlayerAttackConfig::FromJson(attackData); }
    catch (const std::exception&) { rejected = true; }
    require(rejected, "Fixed damage cannot be changed through JSON");
    for (int fps : {30, 60, 120})
    {
        object::ObjectManager objects;
        auto* player = new object::Player(true);
        objects.AddObject(player);
        auto* first = new object::Enemy({672, 2048}, config);
        auto* second = new object::Enemy({352, 2048}, config);
        objects.AddObject(first);
        objects.AddObject(second);
        objects.InitBeforeUpdate();
        // 列挙を逆転させても、同距離は小さい安定IDを選ぶ。
        player->GetReferenceObject([&](uint32_t tag) {
            auto list = objects.GetObjForTag(tag);
            std::reverse(list.begin(), list.end());
            return list;
        });
        std::weak_ptr<object::GameObject> killed = objects.GetObjForTag(object::ENEMY).front();
        dt = 0;
        objects.Update();
        require(first->Health() == 20 && second->Health() == 30, "Inclusive range, immediate attack, stable ID tie");
        dt = 1.0f / fps;
        for (int i = 0; i < fps / 2 - 1; ++i) objects.Update();
        require(first->Health() == 20, "No early repeated damage");
        objects.Update();
        require(first->Health() == 10, "Attack interval 0.5 seconds");
        for (int i = 0; i < fps / 2; ++i) objects.Update();
        require(killed.expired() && objects.GetObjForTag(object::ENEMY).size() == 1,
            "Lethal damage releases enemy through RefreshObjects");
        require(second->Health() == 30, "One attack hits one target only");
        dt = 3;
        objects.Update();
        require(second->Health() == 20, "Long stall fires only once");
        dt = 0;
        objects.Update();
        require(second->Health() == 20, "No stored burst after stall");
        objects.RemoveObject(second);
        objects.Update(); // 表示用の弱参照と取得関数が消失した対象を所有しない。
        std::cout << "PASS: auto attack " << fps << "fps, tie, interval, kill and removal\n";
    }
    {
        object::ObjectManager objects;
        auto* player = new object::Player(true);
        objects.AddObject(player);
        auto* far = new object::Enemy({673, 2048}, config);
        auto* near = new object::Enemy({512, 2048}, config);
        auto* middle = new object::Enemy({612, 2048}, config);
        objects.AddObject(far);
        objects.AddObject(near);
        objects.AddObject(middle);
        objects.InitBeforeUpdate();
        // 最初は射程外だけを列挙。待機後に通常の一覧へ戻して初回即発動を確認する。
        player->GetReferenceObject([&](uint32_t) {
            auto list = objects.GetObjForTag(object::ENEMY);
            list.resize(1);
            return list;
        });
        dt = 3;
        objects.Update();
        require(far->Health() == 30 && near->Health() == 30, "Out of range never damages");
        player->GetReferenceObject([&](uint32_t tag) { return objects.GetObjForTag(tag); });
        dt = 0;
        objects.Update();
        require(near->Health() == 20 && middle->Health() == 30 && far->Health() == 30,
            "Ready without target, nearest target, restart resets timer");
        objects.Update();
        require(near->Health() == 20, "Overlap does not add contact damage or duplicate hits");
    }
    {
        col2d::ColliderManager colliders;
        col2d::ColliderID id;
        {
            object::Enemy enemy({512, 2048}, config);
            enemy.SetColliderManager(&colliders);
            enemy.Init();
            id.index = static_cast<uint32_t>(enemy.TargetId() >> 16);
            id.generation = static_cast<uint16_t>(enemy.TargetId());
            require(enemy.TakeDamage(30) && enemy.Health() == 0 && !enemy.CanBeAttacked(), "Dead enemy is immediately excluded");
            require(!colliders.GetCollider(id)->GetColliderDef()->isActive && !enemy.TakeDamage(10),
                "Lethal damage disables collider and rejects repeated hit");
        }
        require(colliders.GetCollider(id) == nullptr, "Enemy destructor unregisters collider");
    }
    dt = previousDt;
    gameSystem::AppCtx::Deactivate();
}

/// <summary>既存処理を用いて、このテストが担当する回帰条件を検証する。</summary>
/// <returns>全条件が成功すると0。不成立時はrequireから終了コード1で終了する。</returns>
int main() try
{
    VerifyCategoryLifetime();
    VerifyEnemyPursuit();
    VerifyAutoAttack();
    // clampの回帰はテスト用の関数へ切り出さず、実Cameraに対して確認する。
    auto& boundaryCamera = gameSystem::Camera::Instance();
    math::Vector2f farCenter{16960, 4540};
    boundaryCamera.SetTarget(&farCenter, true);
    boundaryCamera.SetWorldSize({16384, 4096});
    boundaryCamera.Update();
    require(boundaryCamera.GetOffset().x == 14464 && boundaryCamera.GetOffset().y == 3016,
        "Actual camera far boundary clamp");
    boundaryCamera.SetWorldSize({640, 480});
    boundaryCamera.Update();
    require(boundaryCamera.GetOffset().x == 0 && boundaryCamera.GetOffset().y == 0,
        "Actual camera small map clamp");
    boundaryCamera.ClearTarget(&farCenter);

    // 実際のRectCollider/Stepを使い、停止・遠方・重なり・追跡で基盤の移動反映を測る。
    // TODO(#6/#7): ObjectManager経由の実操作・描画・シーン遷移を手動確認する。
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
                if (cameraGeometry::BaseMarker({camera.WorldToScreen(baseCenter) - math::Vector2f{48, 32}, {96, 64}}, {{0, 0}, {1920, 1080}}).offscreen)
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

catch (const std::exception& error)
{
    std::cerr << "Unexpected test exception: " << error.what() << '\n';
    return 1;
}
