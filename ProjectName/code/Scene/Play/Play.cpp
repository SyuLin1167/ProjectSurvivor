module;
#include <DxLib.h>

module Scene.Play;

import MyLib.KeyStatus;
import Scene.Title;
import Scene.Result;
import Scene.LevelUp;
import Scene.Relay;
import GameSystem.Window;
import Scene.LoadingScene;
import UI.PlayHud;
import Object.ObjectManager;
import Object.Player;
import Object.Enemy;
import Object.EnemyConfig;
import Object.PlayerAttackConfig;
import Scene.StageConfig;
import MyLib.File.FileSystem;
import Object.Map;
import Object.MovingBase;
import GameSystem.Camera;
import GameSystem.FrameRate;
import Asset.AlphaSprites;
import AppContext;

using namespace gameSystem;

namespace scene
{
    namespace
    {
        /// <summary>新規遠征の依頼を一度読み、読込境界で受注可能かを保証する。</summary>
        /// <param name="id">依頼選択またはRetryからのID。</param>
        /// <returns>検証済みの解放済み依頼。不正時はLoadingSceneの失敗処理へ伝播する。</returns>
        quest::QuestConfig LoadDepartureQuest(const std::string& id)
        {
            auto selected = quest::FindQuest(id);
            const auto& record = AppCtx::Campaign();
            if (!record.Ready() || !quest::IsUnlocked(selected, record.Progress()))
                throw std::logic_error("Cannot depart on a locked quest");
            return selected;
        }

        /// @brief 試行開始時にJSONを読み、設定型の検証を適用する。
        /// @param name 設定ファイルの名前。
        /// @return 検証済みの設定。失敗は読み込み元へ伝播する。
        template<class Config>
        Config LoadConfig(const char* name)
        {
            auto& files = AppCtx::FileSystem();
            return Config::FromJson(files.jsonIO.Load(files.GetDataDir() / name));
        }

        /// @brief ステージで使用する敵設定から試行専用の生成器を作る。
        /// @param world マップ寸法と戦闘モードを含む設定。
        /// @param state 検証済み依頼と選択ノードを持つ遠征状態。
        /// @return 初期状態の生成器。
        object::EnemySpawner CreateSpawner(const stage::StageConfig& world, const expedition::ExpeditionState& state)
        {
            auto& files = AppCtx::FileSystem();
            const auto data = files.jsonIO.Load(files.GetDataDir() / "EnemyConfig");
            auto spawnData = files.jsonIO.Load(files.GetDataDir() / "EnemySpawnConfig");
            // classic/C1では配分を上書きしない。区間開始時だけ確定し、更新へ依頼判定を持ち込まない。
            auto spawn = state.BuildSpawnConfig(spawnData);
            const auto& order = state.EnemyRoster(world.alphaCombat, spawn);
            return object::EnemySpawner(spawn,
                combat::EnemyConfig::FromJson(data), {world.WorldWidth, world.WorldHeight},
                world.alphaCombat ? &data.at("alphaTypes") : nullptr, order);
        }
    }

    Play::Play(std::string id) : Play(expedition::ExpeditionState(LoadDepartureQuest(id), LoadConfig<progression::Config>("ProgressionConfig"))) {}

    Play::Play(expedition::ExpeditionState state) : Play(LoadConfig<stage::StageConfig>("Phase1Config"), std::move(state)) {}

    Play::Play(const stage::StageConfig& world, expedition::ExpeditionState state)
        : m_expedition(std::move(state)), m_stage(world.BaseCenter.x, world.TrialEndX, m_expedition.IsFinalSection()), m_spawner(CreateSpawner(world, m_expedition)),
          m_showNearestEnemy(world.showNearestEnemy)
    {
        if (m_expedition.IsFinalSection())
        {
            auto& files = AppCtx::FileSystem();
            m_boss.emplace(combat::EnemyConfig::BossFromJson(files.jsonIO.Load(files.GetDataDir() / "BossConfig")),
                math::Vector2f{world.WorldWidth, world.WorldHeight});
        }
        m_player = new object::Player(world, LoadConfig<combat::PlayerAttackConfig>("PlayerAttackConfig"));
        m_objManager->AddObject(m_player);
        m_objManager->AddObject(new object::Map(world));
        m_base = new object::MovingBase(world, m_expedition.health, m_expedition.maxHealth);
        m_objManager->AddObject(m_base);
        if (world.alphaCombat)
        {
            m_drops.emplace(m_objManager->Colliders());
            alphaSprites::Load();
        }
    }

    void Play::Init()
    {
        m_objManager->InitBeforeUpdate();
        ApplyGrowth();
        // 全Object初期化後にカメラを合わせ、ロード時間をプレイへ持ち越さない。
        Camera::Instance().Update();
        FrameRate::ResetSimulationTime();
    }

    SceneCmd Play::Start(std::shared_ptr<SceneBase> hold, std::string id)
    {
        return CmdReplace{ [hold = std::move(hold), id]() mutable {
            return std::make_shared<LoadingScene>(LoadPolicy::PROGRESS,
                [id]() { return std::make_shared<Play>(id); }, std::move(hold));
        } };
    }

    SceneCmd Play::Continue(std::shared_ptr<SceneBase> hold, expedition::ExpeditionState state)
    {
        return CmdReplace{ [hold = std::move(hold), state = std::move(state)]() mutable {
            return std::make_shared<LoadingScene>(LoadPolicy::PROGRESS,
                [state]() { return std::make_shared<Play>(state); }, std::move(hold));
        } };
    }

    expedition::ExpeditionState Play::Snapshot() const
    {
        auto state = m_expedition;
        state.health = m_base->Health();
        state.maxHealth = m_base->MaxHealth();
        return state;
    }

    void Play::ApplyGrowth()
    {
        // Playerのコンポーネント生成後に適用し、区間再開でも基礎値から一度だけ計算する。
        m_player->ApplyProgression(m_expedition.growth, m_expedition.CurrentNode().route.damageMultiplier);
        m_experience.SetMagnetRadius(object::ExperienceDrops::MagnetRadius *
            m_expedition.growth.Multiplier(progression::Upgrade::Magnet));
        m_growthDirty = false;
    }

    SceneCmd Play::Update()
    {
        // 結果確定後に再度Updateが呼ばれても、攻撃・移動・遷移を繰り返さない。
        if (m_stage.Finished()) return std::monostate{};
        // 出現不能を戦闘や無期限待機へ紛れ込ませず、操作可能なTitle復帰を残す。
        if (m_boss && m_boss->Failed())
        {
            if (input::KeyStatus::CheckKey(keyType.R, ON_PRESS))
                return CmdReplace{ [] { return std::make_shared<Title>(); } };
            return std::monostate{};
        }
        // 複数レベルの未選択分はシミュレーションを挟まず順番に選ぶ。
        if (m_expedition.growth.Pending() > 0)
        {
            m_growthDirty = true;
            return CmdPush{ [this] { return std::make_shared<LevelUp>(m_expedition.growth); } };
        }
        if (m_growthDirty)
        {
            ApplyGrowth();
        }
        // オブジェクト更新
        m_objManager->Update();

        // Stepの接触被害とLateUpdateがすべて反映された後に判定する。
        const auto end = m_stage.Advance(m_base->Center().x, m_base->Health(),
            input::KeyStatus::CheckKey(keyType.R, ON_PRESS), m_boss && m_boss->Defeated());
        if (end == StageEnd::Success && !m_expedition.IsFinalSection())
            return CmdReplace{ [state = Snapshot()] {
                return std::make_shared<Relay>(state);
            } };
        if (end == StageEnd::Success || end == StageEnd::Defeat)
        {
            // 中継への分岐を通過した最終成功だけ記録する。失敗時も結果画面は表示し再保存を案内する。
            if (end == StageEnd::Success) AppCtx::Campaign().RecordClear(m_expedition.quest.id);
            return CmdReplace{ [result = m_stage.ResultValue(), state = Snapshot()] {
                return std::make_shared<Result>(result, state);
            } };
        }
        if (end == StageEnd::Aborted)
            return CmdReplace{ []() { return std::make_shared<Title>(); } };
        // 終了と期限が同フレームなら上の遷移が優先。Stepの走査中には追加しない。
        // 勝敗判定後にだけ回収・生成を許す。新しい品は次の更新から拾える。
        if (m_drops)
        {
            m_drops->Collect(FrameRate::GetDeltaTime(), *m_player, *m_base);
        }
        m_experience.Update(FrameRate::GetDeltaTime(), m_player->Center());
        // 撃破通知を一度だけ消費し、経験値と従来の特殊アイテムへ振り分ける。
        if (const auto position = m_player->TakeKillPosition())
        {
            m_experience.Add(*position);
            if (m_drops) m_drops->Roll(*position);
        }
        m_expedition.growth.AddExperience(m_experience.TakeExperience());
        SceneCmd next;
        if (m_expedition.growth.Pending() > 0)
        {
            m_growthDirty = true;
            next = CmdPush{ [this] { return std::make_shared<LevelUp>(m_expedition.growth); } };
        }
        // Pushはフレーム終端に適用する。既に進めた移動と生成時計のdtを一致させる。
        m_base->CheckPickup(m_player->Center(), m_player->pData.size);
        if (m_boss && m_base->Center().x >= m_base->EndX())
        {
            // 到達フレームから通常生成を止め、既存敵・アイテム・成長の更新はそのまま続ける。
            m_boss->Start(*m_objManager, *m_base, m_player->Center(), Camera::Instance().VisibleRect());
            if (m_boss->Failed())
            {
                ErrorLogFmtAdd("Boss spawn failed: no safe offscreen candidate within approach time. R returns to Title.");
                return std::monostate{};
            }
        }
        else
        {
            m_spawner.Update(FrameRate::GetDeltaTime(), *m_objManager, *m_base, m_player->Center());
        }
        return next;
    }

    void Play::Draw()
    {
        m_objManager->Draw();
        // 初回描画も含め、そのPlayの実値を読む。共有・静的なHUD状態を残さない。
        const auto result = m_stage.ResultValue();
        auto hud = ui::MakeHudValues(m_base->Health(), result.progress,
            m_stage.Distance(), m_stage.RemainingDistance(), m_base->Center() - m_player->Center(), m_base->MaxHealth());
        // 描画時の中心座標だけを保持し、撃破・再試行をまたいで敵参照を残さない。
        hud.showNearestEnemy = m_showNearestEnemy;
        if (m_showNearestEnemy) hud.toEnemy = ui::NearestEnemyOffset(*m_objManager, m_base->Center());
        m_hud.SetValues(hud, {Camera::Instance().WorldToScreen(m_base->Center() - m_base->Size().Half()), m_base->Size()});
        if (m_drops) m_drops->DrawField();
        m_experience.DrawField();
        m_hud.Draw();
        if (m_boss && m_boss->Started())
        {
            // 通常HUDの右側に専用表示。失敗時も同じ位置で復帰操作を知らせる。
            const int x = int(Window::GetWindowData()->SIZE.x) - 600;
            DrawBox(x - 12, 12, x + 580, 116, GetColor(30, 20, 25), TRUE);
            if (m_boss->Failed())
            {
                DrawString(x, 24, "BOSS SPAWN FAILED", GetColor(255, 160, 100));
                DrawString(x, 58, "No safe position. Press R to return to Title.", GetColor(255, 255, 255));
            }
            else
            {
                DrawString(x, 24, "BOSS BATTLE - DEFEND THE CARAVAN", GetColor(255, 180, 55));
                DrawFormatString(x, 52, GetColor(255, 255, 255), "BOSS HP %.0f / %.0f", m_boss->Health(), m_boss->MaxHealth());
                DrawBox(x, 82, x + 560, 104, GetColor(70, 40, 45), TRUE);
                const int fill = int((m_boss->Health() / m_boss->MaxHealth()) * 560);
                DrawBox(x, 82, x + fill, 104, GetColor(220, 55, 70), TRUE);
            }
        }
        const int growthY = int(Window::GetWindowData()->SIZE.y) - 58;
        DrawFormatString(18, growthY, GetColor(90,220,255), "TOTAL XP: %llu",
            static_cast<unsigned long long>(m_expedition.growth.TotalXp()));
        DrawFormatString(480, growthY, GetColor(230,230,230), "SECTION %d / %d",
            m_expedition.Section(), m_expedition.SectionCount());
        if (m_expedition.growth.Level() == progression::MaxLevel)
            DrawFormatString(180, growthY, GetColor(90,220,255), "Lv.%d MAX", progression::MaxLevel);
        else
            DrawFormatString(180, growthY, GetColor(90,220,255), "Lv.%d  NEXT %llu / %llu",
                m_expedition.growth.Level(), static_cast<unsigned long long>(m_expedition.growth.Xp()),
                static_cast<unsigned long long>(m_expedition.growth.NextRequired()));
        if (m_drops) m_drops->DrawStatus(m_player->BoostSeconds());
        m_base->DrawExploration(ui::PlayHudPanel(m_showNearestEnemy));
    }
}
