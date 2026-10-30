#include <cmath>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <thread>

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
import Scene.LoadingScene;
import Scene.SceneBase;
import Scene.Play;
import Scene.Result;
import Scene.StageState;
import UI.PlayHud;
import <stdexcept>;

/// <summary>回帰条件に失敗した場合は理由を表示してテストを終了する。</summary>
/// <param name="condition">成立を期待する検証条件。</param>
/// <param name="message">条件不成立時に表示する理由。</param>
void require(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

/// <summary>拠点矩形への円の食い込みが誤差範囲内か検証する。</summary>
/// <param name="enemy">半径16uの試作敵。</param>
/// <param name="base">96×64uの試作拠点。</param>
void RequireOutsideBase(const object::Enemy& enemy, const object::MovingBase& base)
{
    const auto center = enemy.Center();
    const auto b = base.Center();
    const math::Vector2f nearest{std::clamp(center.x, b.x - 48, b.x + 48),
        std::clamp(center.y, b.y - 32, b.y + 32)};
    require((center - nearest).Length() >= 15.99f, "Enemy stays outside base rectangle");
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

/// <summary>移動後解決の生成順独立性と、片側だけのトリガーでも押し戻さない契約を検証する。</summary>
void VerifyContactResolution()
{
    for (int trigger = 0; trigger < 3; ++trigger)
        for (bool reverse : {false, true})
        {
            col2d::ColliderManager manager;
            col2d::ColliderDef b{}, e{};
            b.localPos = {720, 2016}; b.isActive = true; b.isTrigger = trigger == 1;
            e.localPos = {832, 2048}; e.isActive = true; e.isTrigger = trigger == 2;
            e.contactAfterMove = true;
            col2d::ColliderID base, enemy;
            if (reverse) enemy = manager.CreateCircleCollider(&e, 16, object::ENEMY);
            base = manager.CreateRectCollider(&b, {96, 64}, object::BASE);
            if (!reverse) enemy = manager.CreateCircleCollider(&e, 16, object::ENEMY);
            manager.AddMask(enemy, col2d::RECT, object::BASE);
            int contacts = 0;
            col2d::ContactListener event;
            event.event = [&]() { ++contacts; };
            manager.AddEvent(enemy, col2d::MakeKey(col2d::RECT, object::BASE), event);
            manager.GetCollider(base)->SetVelocity({2, 0});
            manager.GetCollider(enemy)->SetVelocity({-3, 0});
            manager.Step();
            require(contacts == 1, "Collider event is emitted once in either creation order");
            require(manager.GetCollider(base)->GetColliderDef()->localPos.x == 722, "Base is never pushed back");
            require(manager.GetCollider(enemy)->GetColliderDef()->localPos.x == (trigger ? 829 : 834),
                "Solid enemy is resolved; either trigger suppresses resolution");
        }
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
        object::ObjectManager objects;
        auto* base = new object::MovingBase();
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
        for (int frame = 0; frame < fps * 9; ++frame)
        {
            objects.Update();
            for (const auto& enemy : enemies)
                RequireOutsideBase(*std::dynamic_pointer_cast<object::Enemy>(enemy), *base);
        }
        for (const auto& enemy : enemies)
            require((enemy->AccessPos().NowPos() - base->Center()).Length() < 100, "Three enemies converge on caravan exterior");
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

/// <summary>実Collider・敵・拠点で接触攻撃の間隔、複数敵、破棄と撃破優先を確認する。</summary>
void VerifyContactDamage()
{
    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    const auto config = phase2::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const auto previousDt = dt;
    for (int fps : {30, 60, 120})
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase();
        objects.AddObject(base);
        objects.AddObject(new object::Enemy({832, 2048}, config)); // 正面から接触。
        objects.AddObject(new object::Enemy({704, 2048}, config)); // 背後から接触。
        objects.InitBeforeUpdate();
        require(base->Health() == 100, "New trial resets base HP");
        dt = 0;
        objects.Update();
        require(base->Health() == 80, "Two independent enemies each deal initial 10 damage");
        dt = 1.0f / fps;
        for (int i = 0; i < fps - 1; ++i) objects.Update();
        require(base->Health() == 80, "Continuous contact cannot attack early");
        objects.Update();
        require(base->Health() == 60, "Both enemies repeat at one second");
        require(std::abs(base->Center().x - 848) < 0.1f, "Contact does not impede moving base");
        for (int i = 0; i < fps * 4; ++i)
        {
            objects.Update();
            for (const auto& enemy : objects.GetObjForTag(object::ENEMY))
                RequireOutsideBase(*std::dynamic_pointer_cast<object::Enemy>(enemy), *base);
        }
        require(base->Health() == 0 && std::abs(base->Center().x - 1168) < 0.1f,
            "Object-only update clamps HP; Play owns the stop decision");
        require(!base->TakeDamage(10), "Zero HP rejects additional damage");
        objects.RemoveObject(base);
        objects.Update(); // 接触中の標的破棄後にも弱参照を安全に扱う。
        std::cout << "PASS: contact " << fps << "fps, HP 100->80->60->0, movement and target removal\n";
    }
    {
        // タイマーの再接触・遅延を、実Colliderの移動で再現する。専用の本番APIは増やさない。
        col2d::ColliderManager colliders;
        auto base = std::make_shared<object::MovingBase>();
        base->SetColliderManager(&colliders);
        base->Init();
        object::Enemy enemy({768, 2048}, config);
        enemy.SetColliderManager(&colliders);
        enemy.GetReferenceObject([&](uint32_t) { return std::vector<std::shared_ptr<object::GameObject>>{base}; });
        enemy.Init();
        col2d::ColliderID id;
        id.index = static_cast<uint32_t>(enemy.TargetId() >> 16);
        id.generation = static_cast<uint16_t>(enemy.TargetId());
        auto* circle = dynamic_cast<col2d::CircleCollider*>(colliders.GetCollider(id));
        dt = 0;
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        require(base->Health() == 90, "Initial contact is immediate");
        RequireOutsideBase(enemy, *base); // 中心重なりからも外周へ押し出す。
        for (int i = 0; i < 3; ++i)
        {
            circle->TriggerEvent(col2d::MakeKey(col2d::RECT, object::BASE), {});
            enemy.LateUpdate();
        }
        require(base->Health() == 90, "Duplicate notifications and evaluations cannot multiply damage");
        circle->AddVelocity({0, 200});
        dt = 0.25f;
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        circle->AddVelocity({0, -200});
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        require(base->Health() == 90, "Recontact before cooldown does not reset timer");
        dt = 0.5f;
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        require(base->Health() == 80, "Recontact preserves original one-second cooldown");
        dt = 3;
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        require(base->Health() == 70, "Long contact stall deals only one hit");
        dt = 0;
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        require(base->Health() == 70, "No delayed burst on following frame");
        circle->AddVelocity({0, 200});
        dt = 3;
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        require(base->Health() == 70, "No damage outside contact");
        circle->AddVelocity({0, -200});
        dt = 0;
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        require(base->Health() == 60, "Long absence stores only ready state");
        enemy.LateUpdate();
        require(base->Health() == 60, "No accumulated attacks after recontact");
        base->TakeDamage(55);
        dt = 1;
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
        require(base->Health() == 0, "Damage exceeding remaining HP clamps to zero");
        base.reset();
        enemy.Update(); circle->SetVelocity({}); colliders.Step(); enemy.LateUpdate();
    }
    {
        object::ObjectManager objects;
        auto* player = new object::Player(true);
        auto* base = new object::MovingBase();
        auto* enemy = new object::Enemy({720, 2048}, config);
        objects.AddObject(player); objects.AddObject(base); objects.AddObject(enemy);
        objects.InitBeforeUpdate();
        player->AccessPos().NowPos() = {704, 2032}; // 中心(720,2048)。自動攻撃の判定座標を設定。
        require(enemy->TakeDamage(20), "Prepare lethal player attack");
        dt = 0;
        objects.Update();
        require(base->Health() == 100 && objects.GetObjForTag(object::ENEMY).empty(),
            "Player kill takes priority over contact damage in same frame");
    }
    dt = previousDt;
    gameSystem::AppCtx::Deactivate();
    std::cout << "PASS: recontact, duplicate notifications, stall, lower bound and kill priority\n";
}

/// <summary>通常FPS・停止・復帰・新規開始で時間が蓄積しないことを確認する。</summary>
void VerifySimulationTime()
{
    // 同期構築失敗も例外をゲームループへ漏らさず、未初期化Playへ遷移しない。
    for (auto policy : {scene::LoadPolicy::DONE, scene::LoadPolicy::PROGRESS})
    {
        scene::LoadingScene loading(policy, []() -> std::shared_ptr<scene::SceneBase> {
            throw std::runtime_error("Expected construction failure");
        });
        require(std::holds_alternative<scene::CmdQuit>(loading.Update()),
            "Construction failure safely requests quit");
    }
    gameSystem::SimulationTime clock;
    require(clock.Advance(60, true) == 0, "First frame discards loading time");
    for (int fps : {30, 60, 120})
        require(clock.Advance(1.0f / fps, true) == 1.0f / fps, "Normal FPS unchanged");
    require(clock.Advance(60, true) == 1.0f / 30, "Long stall capped");
    require(clock.Advance(60, false) == 0 && clock.Advance(60, false) == 0,
        "Inactive time not accumulated");
    require(clock.Advance(60, true) == 0, "Resume discards inactive time");
    require(clock.Advance(1.0f / 60, true) == 1.0f / 60, "Resume returns to normal");
    clock.Reset();
    require(clock.Advance(60, true) == 0, "New play discards previous timing");
    gameSystem::FrameRate::ResetSimulationTime();
    require(gameSystem::FrameRate::GetDeltaTime() == 0, "Shared dt cleared on start");
    std::cout << "PASS: simulation time start, resume and frame cap\n";
}

/// <summary>実ObjectManagerを繰り返し生成し、敵・HP・攻撃間隔・参照・カメラの初期状態を確認する。</summary>
void VerifyFreshPlayObjects()
{
    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    const auto config = phase2::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const float previousDt = dt;
    for (int trial = 0; trial < 3; ++trial)
    {
        std::weak_ptr<object::GameObject> oldEnemy;
        {
            object::ObjectManager objects;
            auto* player = new object::Player(true);
            auto* base = new object::MovingBase();
            auto* nearby = new object::Enemy({512, 2048}, config);
            auto* touching = new object::Enemy({768, 2048}, config);
            objects.AddObject(player); objects.AddObject(base);
            objects.AddObject(nearby); objects.AddObject(touching);
            objects.InitBeforeUpdate();
            require(player->Center().x == 512 && base->Center().x == 768 &&
                base->Health() == 100 && nearby->Health() == 30,
                "Fresh positions and HP");
            oldEnemy = objects.GetObjForTag(object::ENEMY).front();
            dt = 0;
            objects.Update();
            require(nearby->Health() == 20 && base->Health() == 90,
                "Fresh player and contact timers are immediately ready");
            const auto initialCamera = gameSystem::Camera::Instance().GetOffset();
            require(initialCamera.x == 0 && initialCamera.y == 1508,
                "Fresh camera follows initial player");
            require(nearby->TakeDamage(100), "Defeat enemy before restart");
            require(base->TakeDamage(100), "Deplete base before restart");
            player->AccessPos().NowPos() = {10000, 2032};
            objects.Update();
            require(objects.GetObjForTag(object::ENEMY).size() == 1 && base->Health() == 0,
                "Mutate previous play before destruction");
        }
        require(oldEnemy.expired(), "Old enemy ownership released");
        // 破棄済みPlayerを追従して参照しないことも実Cameraで通す。
        gameSystem::Camera::Instance().Update();
    }
    dt = previousDt;
    gameSystem::AppCtx::Deactivate();
    std::cout << "PASS: three fresh object lifetimes, HP, timers and camera\n";
}

/// <summary>勝敗・閾値・R競合・結果固定と、実Playの終了から新規開始を検証する。</summary>
void VerifyStageEnd()
{
    using scene::StageEnd;
    for (float endpoint : {5568.0f, 7968.0f})
    {
        scene::StageState stage(768, endpoint);
        require(stage.Distance() == 0 && stage.RemainingDistance() == endpoint - 768 &&
            !stage.Finished(), "Fresh stage progress and result");
        const float before = std::nextafter(endpoint, 0.0f);
        require(stage.Advance(before, 1, false) == StageEnd::None &&
            stage.ResultValue().progress < 1 && stage.RemainingDistance() > 0,
            "Immediately before endpoint is not success or 100 percent");
        require(stage.Advance(endpoint, 1, true) == StageEnd::Success,
            "Exact endpoint success takes priority over R");
        require(stage.Advance(768, 0, true) == StageEnd::None &&
            stage.ResultValue().outcome == StageEnd::Success && stage.ResultValue().health == 1 &&
            stage.ResultValue().progress == 1, "Result emitted once and never overwritten");
        scene::StageState overshoot(768, endpoint);
        require(overshoot.Advance(endpoint + 1, 100, false) == StageEnd::Success &&
            overshoot.RemainingDistance() == 0 && overshoot.Distance() == endpoint - 768,
            "Overshoot clamps progress and remaining distance");
        for (float hp : {0.0f, -1.0f})
        {
            scene::StageState simultaneous(768, endpoint);
            require(simultaneous.Advance(endpoint, hp, true) == StageEnd::Defeat &&
                simultaneous.ResultValue().health == 0, "Defeat precedes success and R");
        }
        scene::StageState defeat(768, endpoint);
        require(defeat.Advance(868, 0, true) == StageEnd::Defeat && defeat.Distance() == 100,
            "Defeat preserves partial progress");
        scene::StageState abort(768, endpoint);
        require(abort.Advance(868, 100, true) == StageEnd::Aborted,
            "R interrupts unfinished play without a win or defeat");
    }

    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    std::filesystem::copy_file("ProjectName/resources/data/EnemyConfig.json",
        files.GetDataDir() / "EnemyConfig.json", std::filesystem::copy_options::overwrite_existing);
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const float previousDt = dt;
    // 固定配置の実Playを無操作で敗北まで進め、Resultへ値だけが渡ることを確認する。
    // そのResultを保持してStart/Retry共通の実LoadingSceneを通し、2回目も同条件に戻す。
    int firstFrames = 0;
    float firstProgress = 0;
    std::shared_ptr<scene::Play> play = std::make_shared<scene::Play>();
    for (int trial = 0; trial < 2; ++trial)
    {
        play->Init();
        dt = 1.0f / 60;
        scene::SceneCmd command;
        int frames = 0;
        while (std::holds_alternative<std::monostate>(command) && frames < 6000)
        {
            command = play->Update();
            ++frames;
        }
        auto* replace = std::get_if<scene::CmdReplace>(&command);
        require(replace != nullptr, "Actual fixed-enemy play reaches result");
        auto result = std::dynamic_pointer_cast<scene::Result>(replace->build());
        require(result && result->ResultValue().outcome == StageEnd::Defeat &&
            result->ResultValue().health == 0 && result->ResultValue().progress < 1,
            "Actual contact damage ends play as defeat");
        for (int frame = 0; frame < 60; ++frame)
            require(std::holds_alternative<std::monostate>(play->Update()),
                "Finished Play never updates or issues another transition");
        const auto snapshot = result->ResultValue();
        if (trial == 0) { firstFrames = frames; firstProgress = snapshot.progress; }
        else require(frames == firstFrames && snapshot.progress == firstProgress,
            "Restart restores HP, positions, attack timers and progress to the same result");
        play.reset();
        command = std::monostate{};
        require(result->ResultValue().progress == snapshot.progress,
            "Result survives old Play destruction by value");
        std::cout << "Stage trial=" << trial << " defeat frame=" << frames
                  << " progress=" << snapshot.progress << " HP=" << snapshot.health << '\n';
        if (trial == 0)
        {
            auto retry = scene::Play::Start(result);
            auto loading = std::get<scene::CmdReplace>(retry).build();
            retry = std::monostate{};
            result.reset();
            scene::SceneCmd ready;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
            while (std::holds_alternative<std::monostate>(ready) && std::chrono::steady_clock::now() < deadline)
            {
                dt = 1.0f / 60;
                ready = loading->Update();
                std::this_thread::yield();
            }
            auto* loaded = std::get_if<scene::CmdReplace>(&ready);
            require(loaded != nullptr, "Shared Start/Retry loading completes");
            play = std::dynamic_pointer_cast<scene::Play>(loaded->build());
            require(play != nullptr, "Retry builds a new Play");
        }
    }
    dt = previousDt;
    gameSystem::AppCtx::Deactivate();
    std::cout << "PASS: stage threshold, defeat/R priority, result latch and actual restart\n";
}

/// <summary>HUDの値と丸め、方向、Pと移動拠点の非貫通を既存Colliderで確認する。</summary>
void VerifyHudAndPlayerContact()
{
    const auto initial = ui::MakeHudValues(100, 0, 0, 4800, {256, 0});
    require(initial.health == 100 && initial.distance == 0 && initial.remaining == 4800 &&
        initial.baseDistance == 256 && !initial.isNear && ui::ProgressPercent(initial.progress) == 0,
        "Initial HUD uses stage values");
    const auto damaged = ui::MakeHudValues(40, 0.5f, 2400, 2400, {0, -128});
    require(damaged.health == 40 && damaged.distance == 2400 && damaged.isNear &&
        ui::ProgressPercent(damaged.progress) == 50 && std::string(ui::DirectionText(damaged.toBase)) == "UP",
        "Damage, travel and NEAR reflect actual values");
    require(!ui::MakeHudValues(0, 0, 0, 4800, {129, 0}).isNear &&
        ui::ProgressPercent(std::nextafter(1.0f, 0.0f)) == 99 && ui::ProgressPercent(1) == 100,
        "HUD boundary never reports early arrival");
    require(std::string(ui::DirectionText({1, 1})) == "DOWN-RIGHT" &&
        std::string(ui::DirectionText({-1, -1})) == "UP-LEFT" &&
        std::string(ui::DirectionText({0, 0})) == "HERE", "HUD world direction convention");

    for (int fps : {30, 60, 120})
        for (bool reverse : {false, true})
            for (int side = 0; side < 5; ++side)
            {
                col2d::ColliderManager manager;
                col2d::ColliderDef b{}, p{};
                b.localPos = {720, 2016}; b.isActive = true; b.isTrigger = false;
                // 右側で静止／左から追突／上下から侵入／同じ深さの角接触を再現する。
                const math::Vector2f starts[] = {{816,2032}, {688,2032}, {752,1984}, {752,2080}, {810,2074}};
                const math::Vector2f speeds[] = {{0,0}, {240,0}, {0,240}, {0,-240}, {0,0}};
                p.localPos = starts[side]; p.isActive = true; p.isTrigger = false; p.contactAfterMove = true;
                col2d::ColliderID base, player;
                if (reverse) player = manager.CreateRectCollider(&p, {32,32}, object::PLAYER);
                base = manager.CreateRectCollider(&b, {96,64}, object::BASE);
                if (!reverse) player = manager.CreateRectCollider(&p, {32,32}, object::PLAYER);
                manager.AddMask(player, col2d::RECT, object::BASE);
                for (int frame = 0; frame < fps; ++frame)
                {
                    manager.GetCollider(base)->SetVelocity({80.0f/fps,0});
                    manager.GetCollider(player)->SetVelocity(speeds[side] / static_cast<float>(fps));
                    manager.Step();
                    const auto bp = manager.GetCollider(base)->GetColliderDef()->localPos;
                    const auto pp = manager.GetCollider(player)->GetColliderDef()->localPos;
                    const float overlapX = (std::min)(bp.x+96, pp.x+32) - (std::max)(bp.x, pp.x);
                    const float overlapY = (std::min)(bp.y+64, pp.y+32) - (std::max)(bp.y, pp.y);
                    require(overlapX <= 0.001f || overlapY <= 0.001f, "Player does not penetrate moving base");
                    require(std::abs(bp.x - (720 + 80.0f*(frame+1)/fps)) < 0.02f && bp.y == 2016,
                        "Player contact never stops or displaces base");
                }
            }
    std::cout << "PASS: HUD values, threshold, direction; player/base contact at 30/60/120fps\n";
}

/// <summary>既存処理を用いて、このテストが担当する回帰条件を検証する。</summary>
/// <returns>全条件が成功すると0。不成立時はrequireから終了コード1で終了する。</returns>
int main() try
{
    VerifyCategoryLifetime();
    VerifyContactResolution();
    VerifyEnemyPursuit();
    VerifyAutoAttack();
    VerifyContactDamage();
    VerifySimulationTime();
    VerifyFreshPlayObjects();
    VerifyStageEnd();
    VerifyHudAndPlayerContact();
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
