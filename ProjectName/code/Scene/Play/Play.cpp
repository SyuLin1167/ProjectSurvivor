module;
#include <DxLib.h>

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
        /// @param selected 生成間隔の任意上書きを持つ検証済み依頼。
        /// @return 初期状態の生成器。
        object::EnemySpawner CreateSpawner(const stage::StageConfig& world, const quest::QuestConfig& selected)
        {
            auto& files = AppCtx::FileSystem();
            const auto data = files.jsonIO.Load(files.GetDataDir() / "EnemyConfig");
            auto spawnData = files.jsonIO.Load(files.GetDataDir() / "EnemySpawnConfig");
            // 上書き後に既存比較モードとの組合せも検証する。
            return object::EnemySpawner(selected.BuildSpawnConfig(spawnData),
                combat::EnemyConfig::FromJson(data), {world.WorldWidth, world.WorldHeight},
                world.alphaCombat ? &data.at("alphaTypes") : nullptr);
        }
    }

    Play::Play(std::string id) : Play(LoadConfig<stage::StageConfig>("Phase1Config"), quest::FindQuest(id)) {}

    Play::Play(const stage::StageConfig& world, quest::QuestConfig selected)
        : m_quest(selected), m_stage(world.BaseCenter.x, world.TrialEndX), m_spawner(CreateSpawner(world, selected)),
          m_showNearestEnemy(world.showNearestEnemy)
    {
        m_player = new object::Player(world, LoadConfig<combat::PlayerAttackConfig>("PlayerAttackConfig"));
        m_objManager->AddObject(m_player);
        m_objManager->AddObject(new object::Map(world));
        m_base = new object::MovingBase(world, m_quest.initialHealth, m_quest.maxHealth);
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

    SceneCmd Play::Update()
    {
        // 結果確定後に再度Updateが呼ばれても、攻撃・移動・遷移を繰り返さない。
        if (m_stage.Finished()) return std::monostate{};
        // オブジェクト更新
        m_objManager->Update();

        // Stepの接触被害とLateUpdateがすべて反映された後に判定する。
        const auto end = m_stage.Advance(m_base->Center().x, m_base->Health(),
            input::KeyStatus::CheckKey(keyType.R, ON_PRESS));
        if (end == StageEnd::Success || end == StageEnd::Defeat)
            return CmdReplace{ [result = m_stage.ResultValue(), selected = m_quest, maxHealth = m_base->MaxHealth()]() {
                return std::make_shared<Result>(result, selected, maxHealth);
            } };
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
        m_base->CheckPickup(m_player->Center(), m_player->pData.size);
        m_spawner.Update(FrameRate::GetDeltaTime(), *m_objManager, *m_base, m_player->Center());
        // 継続
        return std::monostate{};
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
        m_experience.DrawStatus();
        if (m_drops) m_drops->DrawStatus(m_player->BoostSeconds());
        m_base->DrawExploration(ui::PlayHudPanel(m_showNearestEnemy));
    }
}
