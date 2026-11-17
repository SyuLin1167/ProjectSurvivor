export module Scene.Play;
import Scene.SceneBase;
import UI.PlayHud;
import Scene.StageConfig;
import Quest.QuestCatalog;
import MyLib.Math.Vector2;
import Asset.Graph;
import Scene.StageState;
import Object.MovingBase;
import Object.Player;
import Object.CombatDrops;
import Object.EnemySpawner;
import <optional>;

/// <summary>
/// シーン関連
/// </summary>
export namespace scene
{
    /// <summary>
    /// プレイシーンを担当
    /// </summary>
    export class Play final :public SceneBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// @param id 出発前に選んだ依頼。
        explicit Play(quest::Id id);

        /// <summary>
        /// 初期化処理
        /// </summary>
        void Init() override;

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <returns>更新後の遷移要求。継続時は空のコマンド。</returns>
        SceneCmd Update() override;

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw() override;

        /// <summary>StartとRetry共通の読み込み経路で、新しいPlayへの遷移を要求する。</summary>
        /// <param name="hold">読み込み中の表示を保持する元シーン。</param>
        /// <returns>LoadingSceneへ置き換えるコマンド。</returns>
        /// @param id 新規試行に引き継ぐ依頼。
        static SceneCmd Start(std::shared_ptr<SceneBase> hold, quest::Id id);

    private:
        /// @brief 同じ試行の設定で必須オブジェクトを登録する。
        /// @param world 検証済みのステージ設定。
        /// @param selected 今回の依頼定義。
        Play(const stage::StageConfig& world, quest::QuestDefinition selected);
        const quest::QuestDefinition m_quest;
        ui::PlayHud m_hud;
        StageState m_stage;
        object::EnemySpawner m_spawner;
        // Playerと拠点はPlayの終了までObjectManagerに登録され、途中除去されない。
        object::Player* m_player;
        object::MovingBase* m_base;
        std::optional<object::CombatDrops> m_drops;
        bool m_showNearestEnemy;
    };
}
