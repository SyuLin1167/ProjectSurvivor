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
import Object.ExperienceDrops;
import Progression.ProgressionState;
import Expedition.ExpeditionState;
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
        explicit Play(std::string id);
        /// <summary>中継から渡された遠征値で、新しい区間オブジェクトを構築する。</summary>
        /// <param name="state">区間番号が更新済みの成長・HP。</param>
        explicit Play(expedition::ExpeditionState state);

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
        static SceneCmd Start(std::shared_ptr<SceneBase> hold, std::string id);
        /// <summary>成長やHPを再初期化せず、次区間を共通LoadingScene経由で生成する。</summary>
        /// <param name="hold">読込中に表示する中継地点。</param>
        /// <param name="state">開始する区間の遠征値。</param>
        /// <returns>LoadingSceneへの置換。</returns>
        static SceneCmd Continue(std::shared_ptr<SceneBase> hold, expedition::ExpeditionState state);
        /// <summary>現在HPを反映した遠征スナップショットを作る。オブジェクト参照を渡さない。</summary>
        /// <returns>区間遷移先が所有できる値。</returns>
        expedition::ExpeditionState Snapshot() const;

    private:
        /// @brief 同じ試行の設定で必須オブジェクトを登録する。
        /// @param world 検証済みのステージ設定。
        /// @param state 今回の遠征値。
        Play(const stage::StageConfig& world, expedition::ExpeditionState state);
        /// <summary>選択済みの成長倍率をPlayerと磁石へ反映する。初期化後と選択後に呼ぶ。</summary>
        void ApplyGrowth();
        expedition::ExpeditionState m_expedition;
        bool m_growthDirty = false; // 選択後の再開でだけ既存能力を更新する。
        ui::PlayHud m_hud;
        StageState m_stage;
        object::EnemySpawner m_spawner;
        // Playerと拠点はPlayの終了までObjectManagerに登録され、途中除去されない。
        object::Player* m_player;
        object::MovingBase* m_base;
        std::optional<object::CombatDrops> m_drops;
        object::ExperienceDrops m_experience; // 試行専用。敵やColliderの寿命から独立。
        bool m_showNearestEnemy;
    };
}
