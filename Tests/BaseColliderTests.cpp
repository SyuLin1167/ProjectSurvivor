#include <json.hpp>
#include <functional>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
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
import Scene.StageConfig;
import Expedition.ExpeditionState;
import Scene.Relay;
import Scene.LevelUp;
import Quest.QuestCatalog;
import Object.CombatDrops;
import Object.ExperienceDrops;
import Object.PlayerAttackConfig;
import <algorithm>;
import Object.EnemyConfig;
import Object.EnemySpawner;
import Object.SpawnPosition;
import MyLib.File.FileSystem;
import MyLib.File.JsonValue;
import Object.MovingBase;
import Object.ObjectManager;
import GameSystem.FrameRate;
import AppContext;
import Scene.LoadingScene;
import Scene.SceneBase;
import Scene.SceneManager;
import Progression.ProgressionState;
import <stack>;
import Scene.Play;
import Scene.Result;
import Scene.Title;
import Scene.QuestSelect;
import Scene.StageState;
import UI.PlayHud;
import UI.DisplayElements;
import UI.UIManager;
import <stdexcept>;

/// @brief 各ケースが用意した現在のJSONから、本番と同じ検証済み設定を得る。
/// @return テスト中のステージ設定。
stage::StageConfig ReadStageConfig()
{
    auto& files = gameSystem::AppCtx::FileSystem();
    return stage::StageConfig::FromJson(files.jsonIO.Load(files.GetDataDir() / "Phase1Config"));
}

/// @brief テスト用の攻撃設定とステージ設定をプレイヤーに渡す。
/// @return ObjectManagerに所有権を渡す新規個体。
object::Player* CreateStagePlayer()
{
    auto& files = gameSystem::AppCtx::FileSystem();
    return new object::Player(ReadStageConfig(), combat::PlayerAttackConfig::FromJson(
        files.jsonIO.Load(files.GetDataDir() / "PlayerAttackConfig")));
}

/// <summary>回帰条件に失敗した場合は理由を表示してテストを終了する。</summary>
/// <param name="condition">成立を期待する検証条件。</param>
/// <param name="message">条件不成立時に表示する理由。</param>
void require(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

/// <summary>シーン回帰では専用保存先を使い、利用者の攻略記録へ一切触れない。</summary>
void PrepareCampaignFixture()
{
    auto& record = gameSystem::AppCtx::Campaign();
    record = campaign::CampaignSession(std::filesystem::absolute(".verification/campaign-regression.json"));
    require(record.StartNew(), "Create isolated campaign fixture");
    record.RecordClear("pilgrim"); // 既存の両依頼回帰用。解放条件自体は別ケースで検証する。
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
    const auto initial = combat::EnemyConfig::FromJson(source);
    require(initial.Health == 30 && initial.Radius == 16 && initial.Speed == 120, "Committed JSON matches approved defaults");
    auto invalid = source;
    invalid["speed"] = 141;
    bool rejected = false;
    try { combat::EnemyConfig::FromJson(invalid); }
    catch (const std::exception&) { rejected = true; }
    require(rejected, "JSON rejects speed outside delegated range");
    for (int scenario = 0; scenario < 4; ++scenario)
    {
        // 最後の60fpsではメモリ上のJSONだけを変更し、速度と明示配置の反映を実Enemyで測る。
        auto data = source;
        const bool changed = scenario == 3;
        const int fps = scenario == 0 ? 30 : scenario == 2 ? 120 : 60;
        if (changed) data["speed"] = 140;
        const auto config = combat::EnemyConfig::FromJson(data);
        dt = 1.0f / fps;
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base);
        const std::vector<math::Vector2f> positions{{changed ? 1344.0f : 1280.0f, 2048}, {1024,1728}, {1024,2368}};
        for (auto position : positions)
            objects.AddObject(new object::Enemy(position, config));
        objects.InitBeforeUpdate();
        auto enemies = objects.GetObjForTag(object::ENEMY);
        std::weak_ptr<object::GameObject> removed = enemies.front();
        const auto start = enemies.front()->AccessPos().NowPos();
        require(start == positions.front(), "Explicit spawn center reaches actual Enemy");
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
    const auto config = combat::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const auto previousDt = dt;
    auto attackData = files.jsonIO.Load("ProjectName/resources/data/PlayerAttackConfig.json");
    const auto attack = combat::PlayerAttackConfig::FromJson(attackData);
    require(attack.Range == 160 && attack.Damage == 10 && attack.Interval == 0.5f, "Approved attack defaults");
    attackData["damage"] = 11;
    bool rejected = false;
    try { combat::PlayerAttackConfig::FromJson(attackData); }
    catch (const std::exception&) { rejected = true; }
    require(rejected, "Fixed damage cannot be changed through JSON");
    // HP20比較は既存JSON経路で読み込み、実Playerの2発目で敵と判定が除去されることを確認する。
    auto comparisonData = files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json");
    comparisonData["health"] = 20;
    const auto comparison = combat::EnemyConfig::FromJson(comparisonData);
    {
        object::ObjectManager objects;
        objects.AddObject(CreateStagePlayer());
        auto* enemy = new object::Enemy({612, 2048}, comparison);
        objects.AddObject(enemy);
        objects.InitBeforeUpdate();
        std::weak_ptr<object::GameObject> killed = objects.GetObjForTag(object::ENEMY).front();
        dt = 0;
        objects.Update();
        require(enemy->Health() == 10, "HP20 comparison survives first attack");
        dt = 0.5f;
        objects.Update();
        require(killed.expired() && objects.GetObjForTag(object::ENEMY).empty(),
            "HP20 comparison dies on second attack and is released");
        objects.Update(); // 除去後の更新でも攻撃対象の参照を持ち越さない。
    }
    // 実Playerの撃破通知から回収までを通し、プール個体への参照を残さないことを確認。
    {
        const auto path = files.GetDataDir() / "Phase1Config.json";
        auto stage = files.jsonIO.Load(path);
        stage["alphaCombat"] = true;
        std::ofstream(path) << stage.dump();
        object::ObjectManager objects;
        auto* player = CreateStagePlayer();
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(player);
        objects.AddObject(base);
        objects.InitBeforeUpdate();
        object::CombatDrops drops(objects.Colliders());
        const auto variants = comparisonData.at("alphaTypes");
        object::EnemyCatalog catalog(comparison, &variants);
        dt = 0.5f;
        for (int i = 0; i < 48; ++i)
        {
            auto enemy = std::make_shared<object::Enemy>(player->Center(), catalog.Next());
            catalog.Registered();
            objects.AddInitializedObject(enemy);
            for (int frame = 0; frame < 5 && enemy->Health() > 0; ++frame) objects.Update();
            auto position = player->TakeKillPosition();
            require(position.has_value() && !player->TakeKillPosition(), "Single kill event for every alpha type");
            drops.Roll(*position);
            // 満タン中に拾える強化だけ取り除き、後で修理品が残っていることを確認する。
            drops.Collect(dt, *player, *base);
        }
        objects.Update(); // 満HPで残った品の次フレームの接触通知を得る。
        require(base->TakeDamage(25), "Base can take damage before repair");
        drops.Collect(dt, *player, *base);
        require(base->Health() == 100, "Full-health repair stays available and clamps to max");
        // 遠方に残した戦利品で上限に達した条件と切り分け、足元での再取得を確認する。
        object::CombatDrops nearby(objects.Colliders());
        int refreshed = 0;
        for (int i = 0; i < 48 && refreshed < 2; ++i)
        {
            nearby.Roll(player->Center());
            objects.Update(); // 円トリガーとプレイヤー矩形を実際に接触させる。
            nearby.Collect(dt, *player, *base);
            if (player->BoostSeconds() == 10) ++refreshed;
        }
        require(refreshed == 2 && player->BoostSeconds() == 10, "Repeated pickups refresh duration");
        auto target = std::make_shared<object::Enemy>(player->Center(), config);
        objects.AddInitializedObject(target);
        objects.Update();
        require(target->Health() == 15, "Boost deals 15 damage without stacking");
        dt = 0;
        objects.Update();
        require(player->BoostSeconds() == 9.5, "Paused time preserves boost");
        dt = 0.5f;
        for (int i = 0; i < 19; ++i) objects.Update();
        require(player->BoostSeconds() == 0, "Boost expires after ten game seconds");
        std::filesystem::copy_file("ProjectName/resources/data/Phase1Config.json", path,
            std::filesystem::copy_options::overwrite_existing);
        std::cout << "PASS: alpha kill, drop, repair, boost and expiry\n";
    }
    // 個別許可を一般的なHP調整権限へ広げないため、未承認の中間値は拒否する。
    comparisonData["health"] = 25;
    bool healthRejected = false;
    try { combat::EnemyConfig::FromJson(comparisonData); }
    catch (const std::exception&) { healthRejected = true; }
    require(healthRejected, "Unapproved HP25 remains rejected");
    for (int fps : {30, 60, 120})
    {
        object::ObjectManager objects;
        auto* player = CreateStagePlayer();
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
        const auto experiencePosition = player->TakeKillPosition();
        require(experiencePosition.has_value() && !player->TakeKillPosition(), "Kill feeds experience once in classic mode");
        object::ExperienceDrops experience;
        experience.Add(*experiencePosition);
        experience.Update(1, player->Center());
        require(experience.Total() == 1, "Actual kill to magnetic experience pickup");
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
        auto* player = CreateStagePlayer();
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
    const auto config = combat::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const auto previousDt = dt;
    // 同じ更新の敵2体と帰還。角に接する場合と微小な隙間を分け、攻撃間隔も検証する。
    std::filesystem::copy_file("ProjectName/resources/data/ProgressionConfig.json", files.GetDataDir() / "ProgressionConfig.json", std::filesystem::copy_options::overwrite_existing);
    const auto stagePath = files.GetDataDir() / "Phase1Config.json";
    const auto stage = files.jsonIO.Load(stagePath);
    struct PickupCase { bool use520; float pickupY; float gap; bool delivered; };
    // A・Bで同じ帰還経路を使い、Bでは旧地点で拾えないことも確認する。
    for (const auto trial : {PickupCase{false, 2448, 0, true}, PickupCase{false, 2448, 0.125f, false},
                             PickupCase{true, 2568, 0, true}, PickupCase{true, 2448, 0, false}})
    {
        auto selected = stage;
        selected["useExplorationDistance520"] = trial.use520;
        files.jsonIO.Write(stagePath, selected);
        col2d::ColliderManager manager;
        auto base = std::make_shared<object::MovingBase>(ReadStageConfig());
        base->SetColliderManager(&manager);
        base->Init();
        base->CheckPickup({2368, trial.pickupY}, {32, 32});
        object::Enemy front({832, 2048}, config), rear({704, 2048}, config);
        for (auto* enemy : {&front, &rear})
        {
            enemy->SetColliderManager(&manager);
            enemy->GetReferenceObject([&](uint32_t) { return std::vector<std::shared_ptr<object::GameObject>>{base}; });
            enemy->Init();
        }
        col2d::ColliderDef p{};
        p.localPos = {816 + trial.gap, 2080};
        p.isActive = true;
        p.isTrigger = false;
        p.contactAfterMove = true;
        auto player = manager.CreateRectCollider(&p, {32, 32}, object::PLAYER);
        manager.AddMask(player, col2d::RECT, object::BASE);
        dt = 0;
        for (int frame = 0; frame < 2; ++frame)
        {
            front.Update(); rear.Update();
            manager.Step();
            front.LateUpdate(); rear.LateUpdate();
            require(base->Health() == (trial.delivered ? 100 : 80), "Only selected site and actual base contact deliver");
        }
        base->TakeDamage(10);
        require(base->Health() == (trial.delivered ? 90 : 70), "Exactly two hits blocked; repeated contact cannot recharge");
    }
    files.jsonIO.Write(stagePath, stage);
    for (int fps : {30, 60, 120})
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
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
        auto base = std::make_shared<object::MovingBase>(ReadStageConfig());
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
        auto* player = CreateStagePlayer();
        auto* base = new object::MovingBase(ReadStageConfig());
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
    const auto config = combat::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const float previousDt = dt;
    for (int trial = 0; trial < 3; ++trial)
    {
        std::weak_ptr<object::GameObject> oldEnemy;
        {
            object::ObjectManager objects;
            auto* player = CreateStagePlayer();
            auto* base = new object::MovingBase(ReadStageConfig());
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
    PrepareCampaignFixture();
    std::filesystem::copy_file("ProjectName/resources/data/EnemyConfig.json",
        files.GetDataDir() / "EnemyConfig.json", std::filesystem::copy_options::overwrite_existing);
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const float previousDt = dt;
    std::filesystem::copy_file("ProjectName/resources/data/EnemySpawnConfig.json",
        files.GetDataDir() / "EnemySpawnConfig.json", std::filesystem::copy_options::overwrite_existing);
    // 継続出現の実Playを無操作で敗北まで進め、Resultへ値だけが渡ることを確認する。
    // そのResultを保持してStart/Retry共通の実LoadingSceneを通し、2回目も同条件に戻す。
    std::filesystem::copy_file("ProjectName/resources/data/ProgressionConfig.json", files.GetDataDir() / "ProgressionConfig.json", std::filesystem::copy_options::overwrite_existing);
    const auto stagePath = files.GetDataDir() / "Phase1Config.json";
    auto initialStage = files.jsonIO.Load(stagePath);
    require(stage::StageConfig::FromJson(initialStage).alphaCombat, "Shipped stage enables Alpha combat");
    // この遷移テストは描画デバイスを作らないため、試験用コピーだけを画像不要の比較設定にする。
    // Alphaの敵構成・撃破・ドロップは専用の既存ケースで実オブジェクトを検証する。
    initialStage["alphaCombat"] = false;
    files.jsonIO.Write(stagePath, initialStage);
    const auto spawnSource = files.jsonIO.Load("ProjectName/resources/data/EnemySpawnConfig.json");
    // Start/Retry共通のLoadingを基準・候補双方で通し、乱数や時計も新規Playへ持ち越さない。
    std::filesystem::create_directories(files.GetDataDir() / "Quests");
    std::filesystem::copy("ProjectName/resources/data/Quests", files.GetDataDir() / "Quests",
        std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive);
    // 旧C0/C1の遷移回帰はテスト用JSONだけを従来モードへ切り替える。
    // 配布依頼の画面外モードはVerifyOffscreenSpawningで検証する。
    for (const auto& [id, entry] : quest::AvailableQuests)
    {
        const auto path = files.GetDataDir() / entry.file;
        auto data = files.jsonIO.Load(path);
        require(quest::QuestConfig::FromJson(data).BuildSpawnConfig(spawnSource).offscreen,
            "Both shipped quests select offscreen spawning");
        require(quest::QuestConfig::FromJson(data).BuildSpawnConfig(spawnSource).limit == 12,
            "Both shipped quests use twelve enemy cap");
        data["enemySpawn"] = nlohmann::json::object();
        files.jsonIO.Write(path, data);
    }
    // 依頼ごとの開始HP・修理上限・表示値を同じ設定から確認する。
    for (const auto& [id, entry] : quest::AvailableQuests)
    {
        object::ObjectManager objects;
        auto base = std::make_shared<object::MovingBase>(ReadStageConfig(), quest::FindQuest(id).initialHealth, quest::FindQuest(id).maxHealth);
        objects.AddInitializedObject(base);
        const float expected = id == "pilgrim" ? 120.0f : 80.0f;
        require(base->Health() == expected && base->Repair() == 0, "Quest starts at its repair cap");
        require(base->TakeDamage(10) && base->Repair() == 10 && base->Health() == expected,
            "Repair returns exactly the missing quest HP");
        const auto hud = ui::MakeHudValues(base->Health(), 0, 0, 4800, {}, base->MaxHealth());
        require(hud.health == expected && hud.maxHealth == expected, "HUD uses actual quest HP");
    }
    const auto pilgrimPath = files.GetDataDir() / "Quests/Pilgrim.json";
    const auto originalQuest = files.jsonIO.Load(pilgrimPath);
    for (int invalid = 0; invalid < 8; ++invalid)
    {
        auto data = originalQuest;
        if (invalid == 0) data.erase("maxHealth");
        if (invalid == 1) data["initialHealth"] = 121;
        if (invalid == 2) data["destinationId"] = "unknown";
        if (invalid == 3) data["enemySpawn"]["interval"] = "fast";
        if (invalid == 4) data["enemySpawn"]["unexpected"] = 1;
        if (invalid == 5) data["id"] = "";
        if (invalid == 6) data["name"] = "";
        if (invalid == 7) data["description"] = "";
        bool rejected = false;
        try { quest::QuestConfig::FromJson(data); }
        catch (const std::exception&) { rejected = true; }
        require(rejected, "Quest rejects missing, invalid and unsupported settings");
    }
    auto overrideData = originalQuest;
    overrideData["enemySpawn"]["interval"] = 3.5;
    const auto overrideQuest = quest::QuestConfig::FromJson(overrideData);
    auto baseSpawn = spawnSource;
    baseSpawn["balanceSet"] = "baseline";
    const auto merged = overrideQuest.BuildSpawnConfig(baseSpawn);
    require(merged.IntervalAt(0) == 3.5 && merged.firstDelay == 3, "Only allowed interval overrides base settings");
    baseSpawn["balanceSet"] = "candidate";
    baseSpawn["candidate"]["stagedIntervals"] = false;
    baseSpawn["candidate"]["stagedCount"] = true;
    bool incompatible = false;
    try { overrideQuest.BuildSpawnConfig(baseSpawn); }
    catch (const std::exception&) { incompatible = true; }
    require(incompatible, "Incompatible C1 override rejected");
    bool invalidQuestRejected = false;
    try { quest::FindQuest("missing"); }
    catch (const std::invalid_argument&) { invalidQuestRejected = true; }
    require(invalidQuestRejected, "Unknown quest rejected before departure");
    for (const auto selection : {"baseline", "candidate", "countControl", "countCandidate"})
    {
        auto selected = spawnSource;
        const std::string mode = selection;
        selected["balanceSet"] = mode == "baseline" ? "baseline" : "candidate";
        if (mode == "countControl" || mode == "countCandidate")
        {
            selected["candidate"]["stagedIntervals"] = false;
            selected["candidate"]["stagedCount"] = mode == "countCandidate";
        }
        files.jsonIO.Write(files.GetDataDir() / "EnemySpawnConfig.json", selected);
        int firstFrames = 0;
        float firstProgress = 0;
        const auto id = mode == "baseline" ? std::string("pilgrim") : std::string("cargo");
        std::shared_ptr<scene::Play> play = std::make_shared<scene::Play>(id);
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
            require(replace != nullptr, "Actual spawning play reaches result");
            auto result = std::dynamic_pointer_cast<scene::Result>(replace->build());
            require(result && result->ResultValue().outcome == StageEnd::Defeat &&
                result->ResultValue().health == 0 && result->ResultValue().progress < 1,
                "Actual contact damage ends play as defeat");
            for (int frame = 0; frame < 60; ++frame)
                require(std::holds_alternative<std::monostate>(play->Update()),
                    "Finished Play never updates or issues another transition");
            const auto snapshot = result->ResultValue();
            if (trial == 0) { firstFrames = frames; firstProgress = snapshot.progress; }
            else if (mode == "baseline")
                require(frames < firstFrames && snapshot.progress * 1.5f < firstProgress,
                    "Retry rereads quest HP and stage distance");
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
                if (mode == "baseline")
                {
                    // Retry直前に依頼HPと終端を変え、次の試行だけで再読込されることを確認する。
                    auto changedQuest = originalQuest;
                    changedQuest["initialHealth"] = 100;
                    files.jsonIO.Write(pilgrimPath, changedQuest);
                    auto longer = initialStage;
                    longer["trialEndX"] = longer.at("baseCenterX").get<float>() + 7200.0f;
                    files.jsonIO.Write(stagePath, longer);
                }
                auto retry = result->Retry();
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
        files.jsonIO.Write(stagePath, initialStage);
        files.jsonIO.Write(pilgrimPath, originalQuest);
    }
    files.jsonIO.Write(files.GetDataDir() / "EnemySpawnConfig.json", spawnSource);
    auto invalid = initialStage;
    invalid.erase("playerSize");
    files.jsonIO.Write(stagePath, invalid);
    {
        auto start = scene::Play::Start({}, std::string("pilgrim"));
        auto loading = std::get<scene::CmdReplace>(start).build();
        scene::SceneCmd failed;
        for (int frame = 0; frame < 8 && std::holds_alternative<std::monostate>(failed); ++frame)
            failed = loading->Update();
        require(std::holds_alternative<scene::CmdQuit>(failed), "Missing setting stops Play loading");
    }
    files.jsonIO.Write(stagePath, initialStage);
    dt = previousDt;
    gameSystem::AppCtx::Deactivate();
    std::cout << "PASS: stage threshold, defeat/R priority, result latch and actual restart\n";
}

/// <summary>初期化の途中で失敗する個体。登録済みColliderの回収を検証する。</summary>
class FailingSpawn final : public object::GameObject
{
public:
    /// <summary>検証用の破棄結果と登録IDを受け取る。</summary>
    /// <param name="destroyed">デストラクター到達の記録先。</param>
    /// <param name="manager">登録先ColliderManagerの記録先。</param>
    /// <param name="id">登録したIDの記録先。</param>
    FailingSpawn(bool& destroyed, col2d::ColliderManager*& manager, col2d::ColliderID& id)
        : m_destroyed(destroyed), m_manager(manager), m_id(id) {}
    /// <summary>例外時も自分のColliderを回収する。</summary>
    ~FailingSpawn() { if (m_manager) m_manager->Destroy(m_id); m_destroyed = true; }
    /// <summary>Colliderを登録した直後に初期化失敗を再現する。</summary>
    void Init() override
    {
        m_manager = &ColMgr();
        col2d::ColliderDef def{};
        m_id = ColMgr().CreateCircleCollider(&def, 16, object::ENEMY);
        throw std::runtime_error("Expected spawn initialization failure");
    }
    /// <summary>描画を必要としない失敗経路のため何もしない。</summary>
    void Draw() override {}
    /// <summary>既存の敵タグへ分類する。</summary>
    /// <returns>敵のタグ。</returns>
    object::ObjectTag MyObjectTag() const override { return object::ENEMY; }
private:
    bool& m_destroyed;
    col2d::ColliderManager*& m_manager;
    col2d::ColliderID& m_id;
};

/// <summary>C0/C1の数切替、異方向生成、空き枠と安全候補による部分生成を検証する。</summary>
void VerifyStagedCount()
{
    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    auto data = files.jsonIO.Load("ProjectName/resources/data/EnemySpawnConfig.json");
    data["balanceSet"] = "candidate";
    data["candidate"]["stagedIntervals"] = false;
    data["candidate"]["stagedCount"] = true;
    const auto config = object::EnemySpawnConfig::FromJson(data);
    const auto enemy = combat::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    const math::Vector2f world{16384,4096}, distant{16,16};
    require(config.CountAt(19.999) == 1 && config.CountAt(20) == 2, "Count uses scheduled boundary");
    // 同じC1時計で60秒進め、成功生成順・耐久・追跡速度と撃破後のプール再利用を確認する。
    {
        auto alpha = config;
        const auto types = files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json").at("alphaTypes");
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base);
        objects.InitBeforeUpdate();
        object::EnemySpawner spawner(alpha, enemy, world, &types);
        auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
        const auto previousDt = dt;
        dt = 1.0f / 60;
        const float hp[] = {20, 10, 20, 40};
        const float speed[] = {120, 176, 120, 94};
        int spawned = 0;
        for (int frame = 0; frame < 3600; ++frame)
        {
            if (!spawner.Update(dt, objects, *base, distant)) continue;
            const auto items = objects.GetObjForTag(object::ENEMY);
            std::vector<math::Vector2f> before;
            for (const auto& item : items) before.push_back(std::dynamic_pointer_cast<object::Enemy>(item)->Center());
            objects.Update();
            for (size_t i = 0; i < items.size(); ++i)
            {
                const auto unit = std::dynamic_pointer_cast<object::Enemy>(items[i]);
                const int slot = spawned++ % 4;
                require(unit->Health() == hp[slot] &&
                    std::abs(unit->Center().Distance(before[i]) - speed[slot] * dt) < 0.002f,
                    "Alpha mixed roster uses expected HP and pursuit speed");
                unit->TakeDamage(40);
            }
            objects.RefreshObjects();
        }
        require(spawned == 25, "Alpha roster preserves sixty-second C1 schedule");
        dt = previousDt;
    }
    auto invalid = data; invalid["candidate"]["stagedIntervals"] = true;
    bool rejected = false;
    try { object::EnemySpawnConfig::FromJson(invalid); } catch (const std::exception&) { rejected = true; }
    require(rejected, "Count comparison cannot enable staged intervals");

    // 機会ごとに除去して上限見送りを除外し、同一時刻列で数だけが変わることを測る。
    for (bool pair : {false, true})
    for (int fps : {30,60,120})
    {
        auto selected = config; selected.stagedCount = pair;
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        object::EnemySpawner spawner(selected, enemy, world);
        int opportunities = 0, total = 0;
        for (int frame = 1; frame <= 60 * fps; ++frame)
        {
            if (!spawner.Update(1.0f / fps, objects, *base, distant)) continue;
            const double due = 3 + 4 * opportunities;
            require(std::abs(frame / double(fps) - due) <= 1.0 / fps, "C0/C1 fixed four-second schedule");
            const auto items = objects.GetObjForTag(object::ENEMY);
            require(items.size() == (pair && due >= 20 ? 2u : 1u), "19 seconds single, 23 seconds pair");
            if (items.size() == 2)
            {
                const auto a = std::dynamic_pointer_cast<object::Enemy>(items[0])->Center();
                const auto b = std::dynamic_pointer_cast<object::Enemy>(items[1])->Center();
                require(a.Distance(b) >= 64 && a.y != b.y, "Pair has distinct safe directions");
            }
            total += static_cast<int>(items.size());
            for (auto& item : items) std::dynamic_pointer_cast<object::Enemy>(item)->TakeDamage(20);
            objects.RefreshObjects();
            ++opportunities;
        }
        require(opportunities == 15 && total == (pair ? 25 : 15), "C0/C1 expected totals");
        std::cout << "PASS: C" << pair << " " << fps << "fps, 15 opportunities / " << total << " enemies\n";
    }

    // 6/7/8体で2/1/0を生成する。安全候補0/1個も同じ実登録経路で確認する。
    for (int scenario = 0; scenario < 5; ++scenario)
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        object::EnemySpawner spawner(config, enemy, world);
        for (int i = 0; i < 5; ++i)
        {
            require(spawner.Update(i == 0 ? 3.0f : 4.0f, objects, *base, distant), "Prepare through deadline19");
            for (auto& item : objects.GetObjForTag(object::ENEMY))
                std::dynamic_pointer_cast<object::Enemy>(item)->TakeDamage(20);
            objects.RefreshObjects();
        }
        const int existing = scenario < 3 ? 6 + scenario : scenario == 3 ? 2 : 3;
        const std::vector<math::Vector2f> offsets{{480,0},{339.41125f,-339.41125f},{339.41125f,339.41125f}};
        for (int i = 0; i < existing; ++i)
            objects.AddInitializedObject(std::make_unique<object::Enemy>(
                scenario < 3 ? math::Vector2f{100,100.0f + i * 64} : base->Center() + offsets[i], enemy));
        const int expected = scenario < 3 ? 8 - existing : scenario == 3 ? 1 : 0;
        require(spawner.Update(4, objects, *base, distant) == (expected > 0), "Partial success return value");
        require(objects.GetObjForTag(object::ENEMY).size() == existing + expected, "Cap/safe positions bound pair count");
        for (auto& item : objects.GetObjForTag(object::ENEMY))
            std::dynamic_pointer_cast<object::Enemy>(item)->TakeDamage(30);
        objects.RefreshObjects();
        require(!spawner.Update(0.1f, objects, *base, distant), "Partial or skipped count is not queued");
        require(spawner.Update(3.9f, objects, *base, distant) &&
            objects.GetObjForTag(object::ENEMY).size() == 2, "Next opportunity has only its own pair");
        base->TakeDamage(100);
        require(!spawner.Update(100, objects, *base, distant), "Pair mode stops after defeat");
    }
    gameSystem::AppCtx::Deactivate();
}
/// <summary>#19の候補設定、境界周期、固定シード、安全候補と見送り後の再現性を検証する。</summary>
void VerifyBalanceSet()
{
    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    auto source = files.jsonIO.Load("ProjectName/resources/data/EnemySpawnConfig.json");
    const auto baseline = object::EnemySpawnConfig::FromJson(source);
    require(!baseline.healthOverride && !baseline.stagedIntervals && !baseline.randomDirections,
        "Committed selection stays baseline");
    source["balanceSet"] = "candidate";
    const auto config = object::EnemySpawnConfig::FromJson(source);
    const auto enemy = combat::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    const math::Vector2f world{16384,4096}, distant{16,16};
    require(config.healthOverride == 20 && config.seed == 1901 && config.stagedIntervals && config.randomDirections,
        "Candidate JSON selects approved set");
    require(config.IntervalAt(19.999) == 4 && config.IntervalAt(20) == 3 &&
        config.IntervalAt(39.999) == 3 && config.IntervalAt(40) == 2.5, "Stage interval boundary");

    // 個別切替で元に戻せることを確認し、未知のモードや未承認HPは黙って受け入れない。
    auto reverted = source;
    reverted["candidate"]["health"] = 30;
    reverted["candidate"]["stagedIntervals"] = false;
    reverted["candidate"]["randomDirections"] = false;
    const auto restored = object::EnemySpawnConfig::FromJson(reverted);
    require(restored.healthOverride == 30 && !restored.randomDirections && restored.IntervalAt(50) == 4,
        "Individual comparison switches return to baseline behavior");
    for (int invalidCase = 0; invalidCase < 3; ++invalidCase)
    {
        auto invalid = source;
        if (invalidCase == 0) invalid["balanceSet"] = "typo";
        if (invalidCase == 1) invalid["candidate"]["health"] = 25;
        if (invalidCase == 2) invalid["candidate"]["seed"] = -1;
        bool rejected = false;
        try { object::EnemySpawnConfig::FromJson(invalid); } catch (const std::exception&) { rejected = true; }
        require(rejected, "Comparison rejects invalid selection, HP or seed");
    }

    // 機会ごとに撃破して位置見送りを避け、予約時刻と新規Spawnerの同一シード列を比較する。
    const std::vector<double> times{3,7,11,15,19,23,26,29,32,35,38,41,43.5,46,48.5,51,53.5,56,58.5};
    std::vector<int> reference;
    for (int fps : {30,60,120})
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        object::EnemySpawner spawner(config, enemy, world);
        std::vector<int> directions;
        size_t count = 0;
        for (int frame = 1; frame <= fps * 60; ++frame)
        {
            require(!spawner.Update(0, objects, *base, distant), "Pause preserves candidate schedule");
            if (!spawner.Update(1.0f / fps, objects, *base, distant)) continue;
            require(count < times.size() && std::abs(frame / double(fps) - times[count]) <= 1.0 / fps,
                "Candidate preserves scheduled deadlines across 20/40 seconds");
            auto item = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
            require(item->Health() == 20, "Candidate HP reaches spawned Enemy");
            const auto offset = item->Center() - base->Center();
            directions.push_back(offset.y == 0 ? 0 : offset.y < 0 ? 1 : 2);
            item->TakeDamage(20); item.reset(); objects.RefreshObjects();
            ++count;
        }
        require(count == 19, "Candidate has nineteen opportunities");
        // 算法や方向割当を不用意に変えて、同じシードの既存比較を再現不能にしない。
        require(directions == std::vector<int>({2,1,2,1,0,1,1,0,0,0,1,1,1,0,1,1,1,0,0}),
            "Seed1901 matches the recorded direction sequence");
        if (reference.empty()) reference = directions;
        else require(reference == directions, "Seed1901 restarts identically at each frame rate");
        std::cout << "PASS: candidate " << fps << "fps, 19 deadlines and seed1901\n";
    }

    // 上限・位置不成立でも1機会ぶんの乱数を進め、回復直後には埋め戻さない。
    for (bool atCap : {false, true})
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        object::EnemySpawner spawner(config, enemy, world);
        const std::vector<math::Vector2f> offsets{{480,0},{339.41125f,-339.41125f},{339.41125f,339.41125f}};
        for (int i = 0; i < (atCap ? 8 : 3); ++i)
            objects.AddInitializedObject(std::make_unique<object::Enemy>(
                atCap ? math::Vector2f{100,100.0f + i * 64} : base->Center() + offsets[i], enemy));
        require(!spawner.Update(3, objects, *base, distant), "Candidate skips cap or blocked positions");
        for (auto& item : objects.GetObjForTag(object::ENEMY))
            std::dynamic_pointer_cast<object::Enemy>(item)->TakeDamage(30);
        objects.RefreshObjects();
        require(!spawner.Update(0.1f, objects, *base, distant), "Skipped opportunity has no immediate refill");
        require(spawner.Update(3.9f, objects, *base, distant), "Skipped opportunity waits full interval");
        auto item = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
        const auto offset = item->Center() - base->Center();
        require((offset.y == 0 ? 0 : offset.y < 0 ? 1 : 2) == reference[1],
            "Skip consumes same random opportunity as successful spawn");
    }

    // 最優先の候補にプレイヤーがいる場合は別の安全候補を使う。全候補が境界外なら見送る。
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        const std::vector<math::Vector2f> offsets{{480,0},{339.41125f,-339.41125f},{339.41125f,339.41125f}};
        const auto player = base->Center() + offsets[reference[0]];
        object::EnemySpawner spawner(config, enemy, world);
        require(spawner.Update(3, objects, *base, player), "Random order falls back to safe candidate");
        auto item = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
        require(item->Center().Distance(player) >= 256, "Fallback respects player safety");
        item->TakeDamage(20); item.reset(); objects.RefreshObjects();
        object::EnemySpawner outside(config, enemy, {800,4096});
        require(!outside.Update(3, objects, *base, distant), "Random candidates respect map boundary");
        // 大きな遅延でも1体だけ生成し、現在区間の2.5秒を待ってから次の機会へ進む。
        require(spawner.Update(100, objects, *base, distant), "Stall creates only one candidate");
        require(objects.GetObjForTag(object::ENEMY).size() == 1 &&
            !spawner.Update(0.01f, objects, *base, distant), "Stall has no accumulated burst");
        base->TakeDamage(100);
        require(!spawner.Update(100, objects, *base, distant), "Candidate stops after defeat");
    }
    gameSystem::AppCtx::Deactivate();
}
/// <summary>承認された出現条件と実個体の寿命・接近・負荷を既存基盤で確認する。</summary>
void VerifyEnemySpawning()
{
    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    const auto enemy = combat::EnemyConfig::FromJson(files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json"));
    const auto source = files.jsonIO.Load("ProjectName/resources/data/EnemySpawnConfig.json");
    const auto config = object::EnemySpawnConfig::FromJson(source);
    require(config.interval == 4 && config.firstDelay == 3 && config.distance == 480 && config.limit == 8,
        "Committed spawn defaults match approval");
    for (const auto key : {"interval", "firstDelay", "distance", "limit"})
    {
        auto invalid = source; invalid[key] = -1;
        bool rejected = false;
        try { object::EnemySpawnConfig::FromJson(invalid); } catch (const std::exception&) { rejected = true; }
        require(rejected, "Spawn JSON rejects outside approval");
    }
    auto fractional = source; fractional["limit"] = 8.5;
    bool rejected = false;
    try { object::EnemySpawnConfig::FromJson(fractional); } catch (const std::exception&) { rejected = true; }
    require(rejected, "Spawn limit rejects fractional JSON");
    auto changed = source;
    changed["interval"] = 3; changed["firstDelay"] = 2; changed["distance"] = 640; changed["limit"] = 12;
    const auto adjusted = object::EnemySpawnConfig::FromJson(changed);
    require(adjusted.interval == 3 && adjusted.firstDelay == 2 && adjusted.distance == 640 && adjusted.limit == 12,
        "Delegated JSON changes are preserved");
    const math::Vector2f world{16384, 4096}, distantPlayer{16,16};
    // 画面外の単体も対象。撃破直後は一覧から除去される前でも次の生存敵へ切り替える。
    {
        object::ObjectManager objects;
        const math::Vector2f baseCenter{768, 2048};
        require(!ui::NearestEnemyOffset(objects, baseCenter), "Empty scene has no nearest enemy");
        auto far = std::make_shared<object::Enemy>(baseCenter + math::Vector2f{1800, -2400}, enemy);
        objects.AddInitializedObject(far);
        auto offset = ui::NearestEnemyOffset(objects, baseCenter);
        require(offset && offset->Length() == 3000 && std::string(ui::DirectionText(*offset)) == "UP-RIGHT",
            "Offscreen enemy uses base-to-center distance and direction");
        auto close = std::make_shared<object::Enemy>(baseCenter + math::Vector2f{-40, 0}, enemy);
        objects.AddInitializedObject(close);
        offset = ui::NearestEnemyOffset(objects, baseCenter);
        require(offset && offset->x == -40, "Closest living enemy wins");
        close->TakeDamage(30);
        offset = ui::NearestEnemyOffset(objects, baseCenter);
        require(offset && offset->x == 1800, "Dead enemy is immediately excluded");
        far->TakeDamage(30);
        require(!ui::NearestEnemyOffset(objects, baseCenter), "No stale target after final kill");
    }
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const float previousDt = dt;
    for (int fps : {30, 60, 120})
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        object::EnemySpawner spawner(config, enemy, world);
        int count = 0;
        for (int frame = 1; frame <= fps * 60; ++frame)
        {
            require(!spawner.Update(0, objects, *base, distantPlayer), "Paused spawn does not advance");
            if (!spawner.Update(1.0f / fps, objects, *base, distantPlayer)) continue;
            require(std::abs(frame / double(fps) - (3 + 4 * count)) <= 1.0 / fps, "Spawn schedule has no frame drift");
            auto spawned = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
            const auto delta = spawned->Center() - base->Center();
            require(std::abs(delta.Length() - 480) < 0.01f, "Spawn distance from current base");
            require(count % 3 == 0 ? delta.y == 0 : count % 3 == 1 ? delta.y < 0 : delta.y > 0, "Direction cycle");
            spawned->TakeDamage(30); spawned.reset(); objects.RefreshObjects();
            ++count;
        }
        require(count == 15, "Sixty seconds has fifteen opportunities");
        std::cout << "Spawn " << fps << "fps: count=" << count << " times=3..59s\n";
    }
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        object::EnemySpawner spawner(adjusted, enemy, world);
        require(!spawner.Update(1.9f, objects, *base, distantPlayer) && spawner.Update(0.1f, objects, *base, distantPlayer),
            "Adjusted first delay reaches real generation");
        auto first = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
        require(std::abs(first->Center().Distance(base->Center()) - 640) < 0.01f, "Adjusted distance reaches real enemy");
        require(!spawner.Update(2.9f, objects, *base, distantPlayer) && spawner.Update(0.1f, objects, *base, distantPlayer),
            "Adjusted interval reaches real generation");
        base->TakeDamage(100);
        require(!spawner.Update(100, objects, *base, distantPlayer), "No generation after base reaches zero HP");
    }
    // 返却前の領域を上書きせず、同じ領域の再構築で古い弱参照・HP・接触時計を継承しない。
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        auto spawner = std::make_unique<object::EnemySpawner>(config, enemy, world);
        require(spawner->Update(3, objects, *base, distantPlayer), "Pool first acquisition");
        auto first = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
        const auto address = first.get();
        const auto oldId = first->TargetId();
        std::weak_ptr<object::Enemy> old = first;
        dt = 1.0f / 60;
        for (int i = 0; i < 300 && base->Health() == 100; ++i) objects.Update();
        require(base->Health() == 90, "Pooled enemy starts contact timer");
        first->TakeDamage(30); objects.RefreshObjects();
        require(spawner->Update(4, objects, *base, distantPlayer), "Pool uses another slot while old owner remains");
        auto second = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
        require(second.get() != address && !first->CanBeAttacked(), "Owned dead slot is not overwritten");
        first.reset();
        require(old.expired(), "Old weak reference expires before reuse");
        second->TakeDamage(30); second.reset(); objects.RefreshObjects();
        require(spawner->Update(4, objects, *base, distantPlayer), "Returned slot can be acquired again");
        auto reused = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
        require(reused.get() == address && reused->Health() == 30 && reused->CanBeAttacked() &&
            reused->TargetId() != oldId && old.expired(), "Same storage has fresh health, collider identity and weak lifetime");
        // 再構築後も既存の接触処理を通り、Colliderと対象参照が有効であることを確認。
        spawner.reset(); // Spawnerが先に破棄されても管理中の個体・領域は有効。
        dt = 1.0f / 60;
        for (int i = 0; i < 300 && base->Health() == 90; ++i) objects.Update();
        require(base->Health() == 80 && reused->CanBeAttacked(), "Pool storage survives spawner and contacts after reconstruction");
        reused.reset();
    }
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        object::EnemySpawner spawner(config, enemy, world);
        std::vector<std::shared_ptr<object::Enemy>> retained;
        for (int i = 0; i < 12; ++i)
        {
            require(spawner.Update(i == 0 ? 3.0f : 4.0f, objects, *base, distantPlayer), "Pool acquires each free slot");
            auto item = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
            item->TakeDamage(30); objects.RefreshObjects(); retained.push_back(item);
        }
        require(!spawner.Update(4, objects, *base, distantPlayer), "Exhausted pool skips without overwriting retained objects");
        retained.clear();
        require(!spawner.Update(0.1f, objects, *base, distantPlayer) &&
            spawner.Update(3.9f, objects, *base, distantPlayer), "Returned pool waits next interval without backlog");
    }
    // 上限・候補不成立で見送った分を、空きができても即補充しない。
    for (int limit : {8, 12})
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        auto settings = config; settings.limit = limit;
        object::EnemySpawner spawner(settings, enemy, world);
        for (int i = 0; i < limit; ++i)
            objects.AddInitializedObject(std::make_unique<object::Enemy>(math::Vector2f{100, 100.0f + i * 64}, enemy));
        require(!spawner.Update(3, objects, *base, distantPlayer), "At cap skips opportunity");
        { auto dead = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front()); dead->TakeDamage(30); }
        objects.RefreshObjects();
        require(!spawner.Update(0.1f, objects, *base, distantPlayer), "Free slot does not immediately refill");
        require(spawner.Update(3.9f, objects, *base, distantPlayer), "Next interval fills one slot");
        require(objects.GetObjForTag(object::ENEMY).size() == limit, "Live cap is exact");
    }
    {
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        object::EnemySpawner blocked(config, enemy, {800,4096});
        require(!blocked.Update(3, objects, *base, distantPlayer) && objects.GetObjForTag(object::ENEMY).empty(),
            "All three candidates outside map are skipped without clamp");
        object::EnemySpawner spawner(config, enemy, world);
        const auto front = base->Center() + math::Vector2f{480,0};
        require(spawner.Update(3, objects, *base, front), "Player exclusion tries another candidate");
        const auto first = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
        require(first->Center().y < base->Center().y && first->Center().Distance(front) >= 256, "Fallback is upper and safe");
        for (const auto direction : {math::Vector2f{1,0}, math::Vector2f{1,1}.Norm()})
            objects.AddInitializedObject(std::make_unique<object::Enemy>(base->Center() + direction * 480, enemy));
        object::EnemySpawner occupied(config, enemy, world);
        require(!occupied.Update(3, objects, *base, distantPlayer), "All candidates within enemy clearance are skipped");
        for (const auto& item : objects.GetObjForTag(object::ENEMY)) std::dynamic_pointer_cast<object::Enemy>(item)->TakeDamage(30);
        objects.RefreshObjects();
        require(!occupied.Update(0.1f, objects, *base, distantPlayer), "Position skip waits full interval");
        require(occupied.Update(3.9f, objects, *base, distantPlayer), "Position recovery creates next interval");
        // 初期化失敗時の個体と登録を回収し、既存個体の参照は保つ。
        bool destroyed = false; col2d::ColliderManager* colliderManager = nullptr; col2d::ColliderID failedID{};
        try { objects.AddInitializedObject(std::make_unique<FailingSpawn>(destroyed, colliderManager, failedID)); }
        catch (const std::runtime_error&) {}
        require(destroyed && colliderManager && !colliderManager->GetCollider(failedID), "Failed spawn releases object and collider");
        require(base->TakeDamage(1), "Failed spawn does not remove base collider");
        require(objects.GetObjForTag(object::ENEMY).size() == 1, "Failed spawn not exposed in collection");
        // 更新中に保留された個体でも、初期化例外時に指定の解放関数が一度だけ呼ばれる。
        struct DeferredRegistration final : object::GameObject
        {
            std::function<void()> enqueue;
            void Init() override {}
            void Update() override { enqueue(); }
            void Draw() override {}
            object::ObjectTag MyObjectTag() const override { return object::ENEMY; }
        };
        {
            int releases = 0;
            object::ObjectManager deferred;
            auto source = std::make_shared<DeferredRegistration>();
            source->enqueue = [&] {
                deferred.AddObject(new FailingSpawn(destroyed, colliderManager, failedID),
                    [&](object::GameObject* obj) { ++releases; delete obj; });
            };
            deferred.AddInitializedObject(source);
            try { deferred.Update(); }
            catch (const std::runtime_error&) {}
            require(releases == 1 && !deferred.Colliders().GetCollider(failedID) &&
                deferred.GetObjForTag(object::ENEMY).size() == 1, "Deferred failure honors deleter and removes collider");
        }
        require(occupied.Update(100, objects, *base, distantPlayer), "Long stall generates at most one");
        require(objects.GetObjForTag(object::ENEMY).size() == 2 && !occupied.Update(0.01f, objects, *base, distantPlayer),
            "No backlog burst after stall");
    }
    // 形状を含む実接触までの時間。AIは画面内外を参照せず、登録直後はまだ動かない。
    for (int fps : {30,60,120}) for (int direction = 0; direction < 3; ++direction)
    {
        dt = 1.0f / fps;
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        const math::Vector2f offset = direction == 0 ? math::Vector2f{480,0} : math::Vector2f{1, direction == 1 ? -1.0f : 1.0f}.Norm() * 480;
        objects.AddInitializedObject(std::make_unique<object::Enemy>(base->Center() + offset, enemy));
        int frames = 0;
        while (base->Health() == 100 && frames < fps * 10) { objects.Update(); ++frames; }
        require(base->Health() == 90, "Dynamic enemy resolves target and gives first contact damage");
        std::cout << "Approach " << fps << "fps direction=" << direction << " firstContact=" << frames / double(fps) << "s\n";
        objects.RemoveObject(base);
        objects.Update(); // weak_ptrが失効した後の更新も安全。
    }
    // 性能用の限定fixture: HP0でもManager単体を更新し、上限数を60秒相当維持する。
    // 実Playの勝敗後継続を意味しない。30フレームごとに1体を撃破して登録し直す。
    for (int count : {10,20})
    {
        dt = 1.0f / 60;
        object::ObjectManager objects;
        auto* base = new object::MovingBase(ReadStageConfig());
        objects.AddObject(base); objects.InitBeforeUpdate();
        for (int i = 0; i < count; ++i)
            objects.AddInitializedObject(std::make_unique<object::Enemy>(base->Center() + math::Vector2f{96, float(i * 4)}, enemy));
        double total = 0, maximum = 0;
        for (int frame = 0; frame < 3600; ++frame)
        {
            const auto start = std::chrono::steady_clock::now();
            objects.Update();
            if (frame % 30 == 0)
            {
                { auto dead = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front()); dead->TakeDamage(30); }
                objects.RefreshObjects();
                objects.AddInitializedObject(std::make_unique<object::Enemy>(base->Center() + math::Vector2f{96,0}, enemy));
            }
            const double ms = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now() - start).count();
            total += ms; maximum = std::max(maximum, ms);
            require(objects.GetObjForTag(object::ENEMY).size() == count, "Repeated destruction and replacement keeps exact count");
        }
        std::cout << "Load " << count << " enemies, 3600 updates/120 replacements: mean=" << total/3600 << "ms max=" << maximum << "ms (no drawing)\n";
    }
    dt = previousDt;
    gameSystem::AppCtx::Deactivate();
    std::cout << "PASS: dynamic spawn schedule, limits, safety, reference resolution and lifetime\n";
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

/// <summary>描画なしで画面外条件・上限到達・探索時の供給・占有寿命を実生成経路で検証する。</summary>
void VerifyOffscreenSpawning()
{
    gameSystem::AppCtx::Activate();
    auto& files = gameSystem::AppCtx::FileSystem();
    const auto source = files.jsonIO.Load("ProjectName/resources/data/EnemySpawnConfig.json");
    const auto questData = files.jsonIO.Load("ProjectName/resources/data/Quests/Pilgrim.json");
    const auto shipped = quest::QuestConfig::FromJson(questData).BuildSpawnConfig(source);
    // 区間境界の直前・一致時で値を検証する。予約済み時刻は生成時計が保持する。
    for (double time : {0.0, 19.999, 20.0, 44.999, 45.0, 60.0})
    {
        double interval = 2.5;
        int count = 1, limit = 6;
        if (time >= 20) { interval = 2; limit = 8; }
        if (time >= 45) { interval = 1.5; count = 2; limit = 12; }
        require(shipped.IntervalAt(time) == interval && shipped.CountAt(time) == count && shipped.LimitAt(time) == limit,
            "Offscreen approved stage boundaries");
    }
    // 新しい入力境界だけを確認し、旧比較の検証は既存テストへ任せる。
    for (const auto& invalid : {json{{"mode", "offscreen"}, {"limit", 21}},
        json{{"mode", "offscreen"}, {"limit", 10.5}}, json{{"mode", "unknown"}},
        json{{"mode", "classic"}, {"limit", 20}}})
    {
        auto data = questData;
        data["enemySpawn"] = invalid;
        bool rejected = false;
        try { quest::QuestConfig::FromJson(data).BuildSpawnConfig(source); }
        catch (const std::exception&) { rejected = true; }
        require(rejected, "Invalid density override rejected at loading boundary");
    }
    const auto enemies = files.jsonIO.Load("ProjectName/resources/data/EnemyConfig.json");
    const auto enemy = combat::EnemyConfig::FromJson(enemies);
    const auto world = ReadStageConfig();
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const float previousDt = dt;
    auto& camera = gameSystem::Camera::Instance();
    // 毎機会に敵を除去して上限見送りを避け、実生成時計の予約列を独立した期待値と照合する。
    for (int fps : {30,60,120})
    {
        const std::vector<double> expected{3,5.5,8,10.5,13,15.5,18,20.5,22.5,24.5,26.5,28.5,
            30.5,32.5,34.5,36.5,38.5,40.5,42.5,44.5,46.5,48,49.5,51,52.5,54,55.5,57,58.5,60};
        dt = 1.0f / fps;
        object::ObjectManager objects;
        // 時刻検証だけは終端を延長し、終端までに接近不能な候補の正常な見送りと分離する。
        auto scheduleWorld = world;
        scheduleWorld.TrialEndX = 15000;
        auto base = std::make_shared<object::MovingBase>(scheduleWorld);
        objects.AddInitializedObject(base);
        object::EnemySpawner spawner(shipped, enemy, {world.WorldWidth, world.WorldHeight});
        auto tracking = base->Center();
        camera.SetWorldSize({world.WorldWidth, world.WorldHeight});
        camera.SetTarget(&tracking, true);
        camera.Update();
        size_t opportunity = 0;
        for (int frame = 1; frame <= fps * 60; ++frame)
        {
            objects.Update();
            tracking = base->Center();
            camera.Update();
            if (!spawner.Update(dt, objects, *base, tracking)) continue;
            require(opportunity < expected.size() && std::abs(double(frame)/fps - expected[opportunity]) < 0.001,
                "Offscreen reservations persist across 20/45 second boundaries at every frame rate");
            const auto active = objects.GetObjForTag(object::ENEMY);
            size_t expectedCount = 1;
            if (expected[opportunity] >= 45) expectedCount = 2;
            require(active.size() == expectedCount, "Offscreen opportunity uses approved spawn count");
            ++opportunity;
            for (const auto& entry : active) std::dynamic_pointer_cast<object::Enemy>(entry)->TakeDamage(1000);
            objects.RefreshObjects();
        }
        require(opportunity == expected.size(), "Offscreen schedule completes without extra boundary spawns");
        camera.ClearTarget(&tracking);
    }
    for (int limit : {10,12,20}) for (int fps : {30,60,120})
    {
        auto config = quest::QuestConfig::FromJson(questData).BuildSpawnConfig(source);
        config.limit = limit;
        config.Validate();
        dt = 1.0f / fps;
        object::ObjectManager objects;
        // 負荷fixtureだけHPを回復し続け、敗北で測定が途切れないようにする。
        auto base = std::make_shared<object::MovingBase>(world, 1000, 1000);
        objects.AddInitializedObject(base);
        object::EnemySpawner spawner(config, enemy, {world.WorldWidth, world.WorldHeight}, &enemies.at("alphaTypes"));
        math::Vector2f tracking = base->Center();
        camera.SetWorldSize({world.WorldWidth, world.WorldHeight});
        camera.SetTarget(&tracking, true);
        camera.Update();
        std::vector<uint64_t> observed;
        size_t maximum = 0, generated = 0;
        double totalUpdate = 0, maxUpdate = 0, totalSpawn = 0;
        for (int frame = 0; frame < fps * 60; ++frame)
        {
            const auto start = std::chrono::steady_clock::now();
            objects.Update();
            base->Repair();
            tracking = base->Center();
            camera.Update();
            const auto updated = std::chrono::steady_clock::now();
            const auto view = camera.VisibleRect();
            spawner.Update(dt, objects, *base, tracking);
            const auto spawned = std::chrono::steady_clock::now();
            const double ms = std::chrono::duration<double,std::milli>(updated-start).count();
            totalUpdate += ms;
            maxUpdate = std::max(maxUpdate, ms);
            totalSpawn += std::chrono::duration<double,std::milli>(spawned-updated).count();
            const auto active = objects.GetObjForTag(object::ENEMY);
            maximum = std::max(maximum, active.size());
            require(active.size() <= size_t(config.LimitAt((frame+1.0)/fps)) && spawner.OccupiedSlots() <= size_t(limit),
                "Offscreen live and occupied counts stay within limits");
            for (const auto& object : active)
            {
                auto instance = std::dynamic_pointer_cast<object::Enemy>(object);
                if (std::find(observed.begin(), observed.end(), instance->TargetId()) != observed.end()) continue;
                observed.push_back(instance->TargetId());
                ++generated;
                auto visual = enemy;
                visual.look = combat::EnemyConfig::Look::Normal;
                require(spawnPosition::Outside(instance->Center(), visual, view), "Every generated sprite is fully offscreen");
                require(instance->Center().Distance(tracking) >= spawnPosition::PlayerClearance,
                    "Every generated enemy respects player clearance");
            }
        }
        require(maximum == size_t(limit), "Dedicated 60 second fixture reaches target live count");
        // 所有参照の残る撃破個体は生存数から消えても占有する。解放後だけ枠を返す。
        auto held = std::dynamic_pointer_cast<object::Enemy>(objects.GetObjForTag(object::ENEMY).front());
        std::weak_ptr<object::Enemy> weak = held;
        const auto occupied = spawner.OccupiedSlots();
        held->TakeDamage(1000);
        objects.RefreshObjects();
        require(spawner.OccupiedSlots() == occupied && !held->CanBeAttacked(), "Retained dead object still occupies its slot");
        held.reset();
        require(weak.expired() && spawner.OccupiedSlots() == occupied-1, "Last owner releases pool slot");
        std::cout << "Offscreen " << fps << "fps cap=" << limit << " maxLive=" << maximum << " generated=" << generated
            << " updateMean=" << totalUpdate/(fps*60) << "ms updateMax=" << maxUpdate
            << "ms spawnMean=" << totalSpawn/(fps*60) << "ms (no drawing, repaired base)\n";
        camera.ClearTarget(&tracking);
    }
    // 探索時はカメラを拠点から離す。世界四隅でも成立する候補だけを生成する。
    for (const auto target : {math::Vector2f{16,16}, {16368,16}, {16,4080}, {16368,4080}, {2368,2568}})
    {
        object::ObjectManager objects;
        auto base = std::make_shared<object::MovingBase>(world);
        objects.AddInitializedObject(base);
        auto tracking = target;
        camera.SetTarget(&tracking, true);
        camera.Update();
        const auto config = quest::QuestConfig::FromJson(questData).BuildSpawnConfig(source);
        object::EnemySpawner spawner(config, enemy, {world.WorldWidth, world.WorldHeight}, &enemies.at("alphaTypes"));
        bool generated = false;
        for (int frame = 0; frame < 360; ++frame)
            generated = spawner.Update(1.0f/60, objects, *base, tracking) || generated;
        require(generated, "Exploration and world corners retain offscreen route candidates");
        for (const auto& entry : objects.GetObjForTag(object::ENEMY))
        {
            const auto instance = std::dynamic_pointer_cast<object::Enemy>(entry);
            auto visual = enemy; visual.look = combat::EnemyConfig::Look::Normal;
            require(spawnPosition::Outside(instance->Center(), visual, camera.VisibleRect()), "Corner spawning stays outside view");
        }
        camera.ClearTarget(&tracking);
    }
    // 画面がマップ全体を覆う条件では、画面内へ救済生成しない。
    {
        object::ObjectManager objects;
        auto small = world; small.WorldWidth = 640; small.WorldHeight = 480; small.BaseCenter = {320,240};
        auto base = std::make_shared<object::MovingBase>(small);
        objects.AddInitializedObject(base);
        math::Vector2f tracking{320,240};
        camera.SetWorldSize({640,480}); camera.SetTarget(&tracking,true); camera.Update();
        auto config = quest::QuestConfig::FromJson(questData).BuildSpawnConfig(source);
        object::EnemySpawner spawner(config, enemy, {640,480}, &enemies.at("alphaTypes"));
        require(!spawner.Update(3, objects, *base, tracking) && spawner.OccupiedSlots() == 0,
            "No valid offscreen location skips the opportunity");
        camera.ClearTarget(&tracking);
    }
    dt = previousDt;
    gameSystem::AppCtx::Deactivate();
}

/// <summary>既存処理を用いて、このテストが担当する回帰条件を検証する。</summary>
/// <returns>全条件が成功すると0。不成立時はrequireから終了コード1で終了する。</returns>
/// <summary>磁石の境界・停止・行き過ぎ・合算と新規試行の初期値を確認する。</summary>
void VerifyExperienceDrops()
{
    for (int fps : {30, 60, 120})
    {
        object::ExperienceDrops drops;
        drops.Add({160, 0});
        drops.Add({161, 0});
        drops.Update(0, {});
        require(drops.Count() == 2 && drops.Total() == 0, "XP pause preserves state");
        for (int frame = 0; frame < fps; ++frame) drops.Update(1.0f / fps, {});
        require(drops.Total() == 1 && drops.Count() == 1, "XP inclusive magnet boundary and outside");
        drops.Update(1.0f / fps, {161, 0});
        require(drops.Total() == 2 && drops.Count() == 0, "XP coincident pickup without normalization");
        drops.Add({100, 0});
        drops.Update(1, {});
        drops.Update(1, {});
        require(drops.Total() == 3 && drops.Count() == 0, "XP overshoot and duplicate prevention");
    }
    object::ExperienceDrops moving;
    moving.Add({160, 0});
    moving.Update(1.0f / 60, {});
    // 吸引開始後にプレイヤーが離れても追従し、二度目の範囲進入を要求しない。
    moving.Update(2, {-500, 0});
    require(moving.Total() == 1 && moving.Count() == 0, "XP attraction persists outside magnet");
    object::ExperienceDrops full;
    for (int i = 0; i < 300; ++i) full.Add({0, 0});
    require(full.Count() == 256, "XP bounded storage");
    full.Update(1.0f / 60, {});
    require(full.Total() == 300 && full.Count() == 0, "XP cap merge preserves reward");
    object::ExperienceDrops fresh;
    require(fresh.Total() == 0 && fresh.Count() == 0, "XP fresh trial resets all state");
    std::cout << "PASS: experience magnet, pickup, pause, cap and fresh trial\n";
}

/// <summary>成長の保留・候補・能力反映と、入れ子の描画／Pop寿命を確認する。</summary>
void VerifyProgression()
{
    using progression::Upgrade;
    progression::ProgressionState state(progression::Config{});
    state.AddExperience(2);
    require(state.Level() == 1 && state.Pending() == 0, "XP below threshold");
    state.AddExperience(18);
    require(state.Level() == 5 && state.Pending() == 4 && state.Xp() == 2, "Multiple levels retain pending choices");
    for (int i = 0; i < 4; ++i)
    {
        const auto choices = state.Choices();
        require(choices[0] != choices[1] && choices[1] != choices[2] && choices[0] != choices[2], "Unique reproducible choices");
        require(state.Select(choices[0]), "Select available upgrade");
    }
    require(!state.Select(Upgrade::Damage) && state.Pending() == 0, "No duplicate selection without pending level");
    state.AddExperience(100);
    require(state.Level() == 5 && state.Pending() == 0, "Maximum level stops choices");
    gameSystem::AppCtx::Activate();
    object::ObjectManager objects;
    auto world = ReadStageConfig();
    object::Player* player = new object::Player(world, combat::PlayerAttackConfig{160, 10, 0.5f});
    objects.AddObject(player);
    objects.InitBeforeUpdate();
    const float baseDamage = player->AttackDamage();
    const float baseInterval = player->AttackInterval();
    player->ApplyProgression(state);
    require(std::abs(player->AttackDamage() - baseDamage * 1.2f) < 0.001f, "Growth damage");
    require(std::abs(player->AttackInterval() - baseInterval / 1.15f) < 0.001f, "Growth attack speed");
    require(std::abs(player->pData.moveSpeed.x - world.PlayerSpeed * 1.1f) < 0.001f, "Existing movement speed updated");
    player->BoostAttack();
    player->ApplyProgression(state);
    require(std::abs(player->AttackDamage() - baseDamage * 1.2f * 1.5f) < 0.001f, "Growth then temporary boost without compounding");
    object::ExperienceDrops drops;
    drops.SetMagnetRadius(object::ExperienceDrops::MagnetRadius * state.Multiplier(Upgrade::Magnet));
    drops.Add({190, 0});
    drops.Update(1, {});
    require(drops.TakeExperience() == 1 && drops.TakeExperience() == 0 && drops.Total() == 1, "Magnet growth and one-time XP transfer");
    progression::ProgressionState fresh(progression::Config{});
    require(fresh.Level() == 1 && fresh.Xp() == 0 && fresh.Multiplier(Upgrade::Damage) == 1, "New trial resets growth");

    /// <summary>描画順だけを記録する、GPU不要の試験用シーン。</summary>
    struct Layer : scene::SceneBase
    {
        int id;
        bool overlay;
        std::vector<int>& order;
        /// <summary>描画記録先と重ね合わせポリシーを保持する。</summary>
        /// <param name="value">記録するID。</param>
        /// <param name="transparent">下位描画を必要とする場合true。</param>
        /// <param name="draws">この試験中だけ存続する記録先。</param>
        Layer(int value, bool transparent, std::vector<int>& draws) : id(value), overlay(transparent), order(draws) {}
        /// <summary>試験用の追加初期化は不要。</summary>
        void Init() override {}
        /// <summary>試験用シーンは遷移しない。</summary>
        /// <returns>継続コマンド。</returns>
        scene::SceneCmd Update() override { return {}; }
        /// <summary>構築時の描画方針を返す。</summary>
        /// <returns>背景を描く場合true。</returns>
        bool DrawsBelow() const override { return overlay; }
        /// <summary>描画順を記録する。</summary>
        void Draw() override { order.push_back(id); }
    };
    std::vector<int> order;
    std::stack<std::shared_ptr<scene::SceneBase>> stack;
    stack.push(std::make_shared<Layer>(0, false, order));
    std::weak_ptr<scene::SceneBase> background = stack.top();
    for (int repeat = 0; repeat < 3; ++repeat)
    {
        stack.push(std::make_shared<Layer>(1, true, order));
        stack.push(std::make_shared<Layer>(2, true, order));
        order.clear();
        scene::DrawSceneLayers(stack);
        require(order == std::vector<int>({0,1,2}), "Nested overlays draw back to front");
        scene::PopOverlay(stack);
        scene::PopOverlay(stack);
        scene::PopOverlay(stack);
        require(stack.size() == 1 && !background.expired(), "Pop preserves root ownership");
        require(gameSystem::FrameRate::GetDeltaTime() == 0, "Pop resets simulation time");
    }
    stack.push(std::make_shared<Layer>(3, false, order));
    order.clear();
    scene::DrawSceneLayers(stack);
    require(order == std::vector<int>({3}), "Opaque scene hides lower layers");
    std::cout << "PASS: progression, ability updates, XP transfer and overlay layers/lifetime\n";
}

/// <summary>Start/Continue/Retryの実LoadingSceneを完了まで進める。</summary>
/// <param name="command">LoadingSceneへの置換要求。</param>
/// <returns>まだInitしていない新しいPlay。</returns>
std::shared_ptr<scene::Play> LoadExpeditionPlay(scene::SceneCmd command)
{
    auto loading = std::get<scene::CmdReplace>(command).build();
    command = std::monostate{};
    scene::SceneCmd ready;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    while (std::holds_alternative<std::monostate>(ready) && std::chrono::steady_clock::now() < deadline)
    {
        dt = 1.0f / 60;
        ready = loading->Update();
        std::this_thread::yield();
    }
    require(std::holds_alternative<scene::CmdReplace>(ready), "Expedition loading completes");
    auto play = std::dynamic_pointer_cast<scene::Play>(std::get<scene::CmdReplace>(ready).build());
    require(play != nullptr, "Expedition loading creates Play");
    return play;
}

/// <summary>両依頼の遠征値・実シーン遷移・区間別敗北とRetryを既存の読込経路で確認する。</summary>
void VerifyExpedition()
{
    gameSystem::AppCtx::Activate();
    PrepareCampaignFixture();
    auto& files = gameSystem::AppCtx::FileSystem();
    auto& dt = const_cast<float&>(gameSystem::FrameRate::GetDeltaTime());
    const auto stagePath = files.GetDataDir() / "Phase1Config.json";
    const auto original = files.jsonIO.Load(stagePath);
    auto headless = original;
    headless["alphaCombat"] = false; // 描画なしの既存テスト条件。配布設定は変更しない。
    files.jsonIO.Write(stagePath, headless);
    for (const auto id : {"pilgrim", "cargo"})
    {
        auto& campaign = gameSystem::AppCtx::Campaign();
        require(campaign.StartNew(), "Fresh campaign for expedition unlock test");
        if (std::string(id) == "cargo") campaign.RecordClear("pilgrim");
        expedition::ExpeditionState initial(quest::FindQuest(id), progression::Config{});
        initial.health -= 30;
        initial.growth.AddExperience(8);
        require(initial.growth.Select(progression::Upgrade::Damage) && initial.growth.Select(progression::Upgrade::AttackSpeed),
            "Expedition fixture selects two upgrades");
        auto play = std::make_shared<scene::Play>(initial);
        play->Init();
        require(play->Snapshot().section == 1 && play->Snapshot().health == initial.health,
            "First section uses current health without full repair");

        // 遷移テストだけdtを大きくして終端へ進める。実時間60秒の試遊や負荷測定ではない。
        dt = 60.1f;
        auto command = play->Update();
        require(std::holds_alternative<scene::CmdReplace>(command), "First success requests relay");
        auto relay = std::dynamic_pointer_cast<scene::Relay>(std::get<scene::CmdReplace>(command).build());
        require(relay && relay->State().section == 1 && relay->State().health == initial.health,
            "First success retains health in relay");
        require(!campaign.Progress().Cleared(id), "First section does not record a campaign clear");
        std::weak_ptr<scene::Play> oldPlay = play;
        play.reset();
        command = std::monostate{};
        require(oldPlay.expired(), "Relay does not retain old Play or its objects");
        gameSystem::Camera::Instance().Update(); // 古いPlayerのカメラ参照が解除済みであることも通す。
        dt = 1.0f / 60;
        for (int frame = 0; frame < 120; ++frame)
            require(std::holds_alternative<std::monostate>(relay->Update()), "Relay waits for explicit input");
        require(relay->State().health == initial.health && relay->State().growth.TotalXp() == 8,
            "Relay wait does not advance combat or progression");
        play = LoadExpeditionPlay(relay->Continue());
        require(std::holds_alternative<std::monostate>(relay->Continue()), "Relay cannot start twice");
        relay.reset();
        play->Init();
        const auto continued = play->Snapshot();
        require(continued.section == 2 && continued.health == initial.health && continued.maxHealth == initial.maxHealth &&
            continued.growth.Level() == 3 && continued.growth.Xp() == 1 && continued.growth.TotalXp() == 8 &&
            continued.growth.Multiplier(progression::Upgrade::Damage) == initial.growth.Multiplier(progression::Upgrade::Damage),
            "Second section preserves HP, XP and upgrades");
        dt = 1.0f / 60;
        require(std::holds_alternative<std::monostate>(play->Update()), "Second section starts unfinished with fresh clocks");
        dt = 60.1f;
        command = play->Update();
        auto result = std::dynamic_pointer_cast<scene::Result>(std::get<scene::CmdReplace>(command).build());
        require(result && result->ResultValue().outcome == scene::StageEnd::Success && result->State().section == 2,
            "Second success completes expedition");
        require(campaign.Progress().Cleared(id) && !campaign.Dirty(), "Final success records and saves clear");
        play.reset();
        command = std::monostate{};
        play = LoadExpeditionPlay(result->Retry());
        result.reset();
        play->Init();
        const auto fresh = play->Snapshot();
        require(campaign.Progress().Cleared(id), "Retry preserves campaign clear");
        require(fresh.section == 1 && fresh.growth.Level() == 1 && fresh.growth.Xp() == 0 && fresh.growth.TotalXp() == 0 &&
            fresh.growth.Pending() == 0 && fresh.health == fresh.quest.initialHealth && fresh.maxHealth == fresh.quest.maxHealth,
            "Retry starts new expedition with quest HP and no growth");
        play.reset();

        // 両区間とも実際の敵生成・接触で敗北させ、中継へ誤遷移しないことを確認する。
        for (int section : {1,2})
        {
            require(campaign.StartNew(), "Clear campaign before defeat case");
            auto vulnerable = initial;
            vulnerable.section = section;
            vulnerable.health = 1;
            play = std::make_shared<scene::Play>(vulnerable);
            play->Init();
            dt = 1.0f / 60;
            scene::SceneCmd ended;
            for (int frame = 0; frame < 3600 && std::holds_alternative<std::monostate>(ended); ++frame) ended = play->Update();
            require(std::holds_alternative<scene::CmdReplace>(ended), "Each section can reach defeat");
            auto defeat = std::dynamic_pointer_cast<scene::Result>(std::get<scene::CmdReplace>(ended).build());
            require(defeat && defeat->ResultValue().outcome == scene::StageEnd::Defeat && defeat->State().section == section &&
                defeat->State().health == 0, "Defeat retains correct section and zero HP");
            require(campaign.Progress().cleared.empty(), "Neither section defeat unlocks a quest");
            play.reset();
        }
        // 遠征値に未処理選択があっても捨てず、開始直後は戦闘より先にオーバーレイへ送る。
        auto pending = initial;
        pending.Advance();
        pending.growth.AddExperience(4);
        play = std::make_shared<scene::Play>(pending);
        play->Init();
        dt = 1.0f / 60;
        auto choice = play->Update();
        require(std::holds_alternative<scene::CmdPush>(choice) && play->Snapshot().growth.Pending() == 1,
            "Pending selection survives continuation and pauses before simulation");
        auto overlay = std::dynamic_pointer_cast<scene::LevelUp>(std::get<scene::CmdPush>(choice).build());
        require(overlay && overlay->DrawsBelow(), "Continuation uses existing background overlay");
        overlay.reset();
        choice = std::monostate{};
        play.reset();
    }
    files.jsonIO.Write(stagePath, original);
    std::cout << "PASS: two-section expedition, relay, growth/HP carry, section defeats and fresh retry\n";
}

/// <summary>実保存・再読込とTitleの確認経路を検証し、利用者のLocalAppDataは使わない。</summary>
void VerifyCampaign()
{
    gameSystem::AppCtx::Activate();
    auto& record = gameSystem::AppCtx::Campaign();
    const auto path = std::filesystem::absolute(L".verification/攻略記録/campaign.json");
    std::filesystem::create_directories(path.parent_path());
    std::filesystem::remove(path); // テスト専用の単一ファイルだけ初期化する。
    record = campaign::CampaignSession(path);
    const auto ids = quest::KnownIds();
    record.Load(ids);
    require(record.Status() == campaign::SaveStatus::Missing && !record.Ready(), "Missing save disables Continue");
    {
        auto title = std::make_shared<scene::Title>();
        require(std::holds_alternative<std::monostate>(title->ContinueGame()), "Missing save cannot continue");
        auto next = title->RequestNewGame();
        require(std::holds_alternative<scene::CmdReplace>(next) && !title->Confirming(), "First new game needs no overwrite");
        require(std::dynamic_pointer_cast<scene::QuestSelect>(std::get<scene::CmdReplace>(next).build()) != nullptr,
            "New game reaches QuestSelect");
    }
    const auto pilgrim = quest::FindQuest("pilgrim"), cargo = quest::FindQuest("cargo");
    require(quest::IsUnlocked(pilgrim, record.Progress()) && !quest::IsUnlocked(cargo, record.Progress()),
        "Only pilgrim unlocked initially");
    bool rejected = false;
    try { auto locked = std::make_shared<scene::Play>("cargo"); } catch (const std::logic_error&) { rejected = true; }
    require(rejected, "Departure boundary rejects locked cargo");
    record.RecordClear("pilgrim");
    require(!record.Dirty() && quest::IsUnlocked(cargo, record.Progress()), "Clear saves and unlocks cargo");
    campaign::CampaignSession restarted(path);
    restarted.Load(ids);
    require(restarted.Ready() && restarted.Progress().Cleared("pilgrim"), "New session restores unlock from disk");

    // 確認を開く/取り消す経路は、ファイルの更新時刻も攻略記録も変えない。
    {
        auto title = std::make_shared<scene::Title>();
        const auto timestamp = std::filesystem::last_write_time(path);
        require(std::holds_alternative<std::monostate>(title->RequestNewGame()) && title->Confirming(), "Existing save asks confirmation");
        title->ConfirmNewGame(false);
        require(record.Progress().Cleared("pilgrim") && std::filesystem::last_write_time(path) == timestamp,
            "Cancel keeps memory and save unchanged");
        require(std::holds_alternative<scene::CmdReplace>(title->ContinueGame()), "Continue accepts valid record");
        title->RequestNewGame();
        require(std::holds_alternative<scene::CmdReplace>(title->ConfirmNewGame(true)), "Confirmed new game proceeds");
        restarted.Load(ids);
        require(record.Progress().cleared.empty() && restarted.Progress().cleared.empty(), "Confirmed reset replaces saved progress");
    }

    // ファイルは存在するが無効な場合、読込や取消で黙って修復・上書きしない。
    for (const auto contents : {"{broken", "{\"version\":99,\"clearedQuests\":[]}",
        "{\"version\":1,\"clearedQuests\":[\"unknown\"]}", "{\"version\":1,\"clearedQuests\":42}"})
    {
        { std::ofstream output(path, std::ios::binary); output << contents; }
        record.Load(ids);
        require(!record.Ready() && record.Status() != campaign::SaveStatus::Missing, "Invalid record differs from absence");
        if (std::string(contents).find("99") != std::string::npos)
            require(record.Status() == campaign::SaveStatus::Unsupported, "Unknown version is distinguished");
        auto title = std::make_shared<scene::Title>();
        const auto timestamp = std::filesystem::last_write_time(path);
        require(std::holds_alternative<std::monostate>(title->ContinueGame()), "Corrupt save cannot continue");
        title->RequestNewGame();
        require(title->Confirming(), "Invalid existing record still requires overwrite consent");
        title->ConfirmNewGame(false);
        std::ifstream input(path, std::ios::binary);
        const std::string retained((std::istreambuf_iterator<char>(input)), {});
        require(retained == contents && std::filesystem::last_write_time(path) == timestamp, "Invalid data preserved on cancel");
    }

    // 書込不能は一時パスを空ディレクトリで塞いで再現する。旧本体は読める状態を保つ。
    require(record.StartNew(), "Explicit reset can replace corrupt data");
    const auto temporary = std::filesystem::path(path.wstring() + L".tmp");
    std::filesystem::create_directory(temporary);
    record.RecordClear("pilgrim");
    require(record.Dirty() && record.Progress().Cleared("pilgrim"), "Write failure retains unsaved achievement");
    restarted.Load(ids);
    require(restarted.Ready() && restarted.Progress().cleared.empty(), "Failed save retains old complete file");
    require(!record.StartNew() && record.Progress().Cleared("pilgrim"), "Failed reset preserves live record");
    record.Load(ids);
    require(record.Progress().Cleared("pilgrim"), "Reload does not roll back unsaved achievement");
    std::filesystem::remove(temporary);
    require(record.Save() && !record.Dirty(), "Save retry commits retained achievement");
    restarted.Load(ids);
    require(restarted.Progress().Cleared("pilgrim"), "Successful retry persists achievement");
    // 中断された一時書込があっても、本体からだけ復元する。
    { std::ofstream output(temporary, std::ios::binary); output << "partial"; }
    restarted.Load(ids);
    require(restarted.Progress().Cleared("pilgrim"), "Interrupted temporary file is not loaded");
    std::filesystem::remove(temporary);
    std::filesystem::remove(path);
    gameSystem::AppCtx::Deactivate();
    std::cout << "PASS: campaign unlock, Unicode save, reload, confirmation/cancel, corruption and write failure\n";
}

/// <summary>取得ヘルパーの欠落・型不正・既定値を確認する。仕様固有の範囲は既存設定回帰で検証する。</summary>
void VerifyJsonValue()
{
    const json data{{"hp", 120}, {"speed", 80.0}, {"name", "pilgrim"}, {"enabled", true}, {"nullValue", nullptr}};
    require(file::jsonValue::Read<int>(data, "hp") == 120 &&
        file::jsonValue::Read<float>(data, "speed") == 80 &&
        file::jsonValue::Read<std::string>(data, "name") == "pilgrim" &&
        file::jsonValue::Read<bool>(data, "enabled"), "JSON helper reads multiple types");
    require(!file::jsonValue::ReadOptional<int>(data, "missing") &&
        file::jsonValue::ReadOr(data, "missing", 42) == 42 &&
        file::jsonValue::ReadOr(data, "hp", 42) == 120, "Defaults only apply to absent keys");
    for (const auto key : {"missing", "name", "nullValue"})
    {
        bool rejected = false;
        try { file::jsonValue::Read<int>(data, key); }
        catch (const std::runtime_error& error) { rejected = std::string(error.what()).find(key) != std::string::npos; }
        require(rejected, "Required missing/invalid values report the field name");
    }
    bool rejected = false;
    try { file::jsonValue::ReadOr(data, "nullValue", 42); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "Present null must not silently use a default");
    rejected = false;
    try { file::jsonValue::ReadOptional<int>(json::array(), "hp"); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "Optional reads reject a non-object parent");
    // 値の正常性も取得境界で拒否し、省略時だけ検証を実施しない。
    const json checked = {{"name", ""}, {"interval", 2}, {"valid", "road"}};
    require(file::jsonValue::Read<std::string>(checked, "valid", file::jsonValue::NonEmpty, "not empty") == "road",
        "Validated JSON read");
    require(!file::jsonValue::ReadOptional<std::string>(checked, "absent", file::jsonValue::NonEmpty, "not empty"),
        "Absent optional skips value validation");
    for (const auto key : {"name", "interval"})
    {
        bool rejected = false;
        try
        {
            if (std::string(key) == "name")
                (void)file::jsonValue::Read<std::string>(checked, key, file::jsonValue::NonEmpty, "not empty");
            else
                (void)file::jsonValue::ReadOptional<int>(checked, key, [](int value) { return value >= 3; }, "at least 3");
        }
        catch (const std::runtime_error& error) { rejected = std::string(error.what()).find(key) != std::string::npos; }
        require(rejected, "Value validation identifies rejected field");
    }
    std::cout << "PASS: JSON typed reads, optional/default semantics and field diagnostics\n";
}

int main() try
{
    VerifyJsonValue();
    VerifyCategoryLifetime();
    VerifyContactResolution();
    VerifyEnemyPursuit();
    VerifyAutoAttack();
    VerifyContactDamage();
    VerifySimulationTime();
    VerifyFreshPlayObjects();
    VerifyStageEnd();
    VerifyExpedition();
    VerifyCampaign();
    VerifyStagedCount();
    VerifyBalanceSet();
    VerifyEnemySpawning();
    VerifyOffscreenSpawning();
    VerifyExperienceDrops();
    VerifyProgression();
    // UIManager経由の再利用要素とClear後の値リセットを検証する。描画は手動確認する。
    ui::UIManager hudManager;
    const auto labelID = hudManager.Create<ui::TextLabel>("HP 50%");
    auto* label = dynamic_cast<ui::TextLabel*>(hudManager.GetElement(labelID));
    require(label && label->Text() == "HP 50%", "UI text is preserved literally");
    label->SetText("HP 0%");
    require(label->Text() == "HP 0%", "UI text replacement");
    const auto gaugeID = hudManager.Create<ui::Gauge>();
    auto* gauge = dynamic_cast<ui::Gauge*>(hudManager.GetElement(gaugeID));
    gauge->SetValue(25, 50);
    require(gauge->Ratio() == 0.5f, "Gauge supports non-HP maximum");
    gauge->SetValue(60, 50);
    require(gauge->Ratio() == 1, "Gauge clamps overflow");
    gauge->SetValue(-10, 50);
    require(gauge->Ratio() == 0, "Gauge clamps underflow");
    gauge->SetValue(10, 0);
    require(gauge->Ratio() == 0, "Gauge invalid maximum is empty");
    hudManager.Clear();
    require(!hudManager.GetElement(labelID) && !hudManager.GetElement(gaugeID), "HUD IDs expire on clear");
    const auto freshGauge = hudManager.Create<ui::Gauge>();
    require(dynamic_cast<ui::Gauge*>(hudManager.GetElement(freshGauge))->Ratio() == 0, "Fresh gauge has no previous state");
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
                b->SetVelocity(movement::BaseDisplacement(center, 15000, 80, 1.0f / fps));
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
