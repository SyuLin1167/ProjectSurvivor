export module Object.MovingBase;
import Object.GameObject;
import Component.CombatEffect;
import Scene.StageConfig;
import MyLib.Shape.Rect;

export namespace object
{
    /// <summary>プレイヤーを待たずに直進する試作拠点。勝敗判定はPlayが担当する。</summary>
    class MovingBase final : public GameObject
    {
    public:
        /// <summary>試行の初期HPで拠点と探索地点を生成する。</summary>
        /// <param name="config">検証済みのステージ設定。</param>
        /// @param initialHealth 検証済みの開始HP。
        /// <param name="maxHealth">検証済みの修理上限。</param>
        explicit MovingBase(const stage::StageConfig& config, float initialHealth = 100, float maxHealth = 100);
        /// <summary>所有するコライダーを解放する。</summary>
        ~MovingBase();
        /// <summary>承認された中心座標から拠点矩形とコライダーを初期化する。</summary>
        void Init() override;
        /// <summary>時間に応じた前進量をコライダーへ渡す。</summary>
        void Update() override;
        /// <summary>衝突処理後の座標を読み戻す。</summary>
        void LateUpdate() override;
        /// <summary>可視の拠点矩形を描画する。画面座標のHUDはPlayが担当する。</summary>
        void Draw() override;
        /// <summary>拠点の更新・参照に使用するタグを返す。</summary>
        /// <returns>拠点を識別するObjectTag::BASE。</returns>
        ObjectTag MyObjectTag() const override { return ObjectTag::BASE; }

        /// <summary>追跡対象として現在の中心位置を返す。</summary>
        /// <returns>キャラバン中心のワールド座標（u）。</returns>
        math::Vector2f Center() const { return m_pos + m_config.BaseSize.Half(); }
        /// <summary>試作の被害を下限0で適用する。ゲーム更新の停止はPlayが担当する。</summary>
        /// <param name="damage">正の有限ダメージ。</param>
        /// <returns>有効な拠点の攻撃が成立した場合（無効化を含む）true。</returns>
        bool TakeDamage(float damage);
        /// <summary>試作アイテムで生存中の拠点を最大HPまで修理する。</summary>
        /// <returns>実際の回復量。満タン・破壊済みなら0。</returns>
        float Repair();
        /// <summary>地点とプレイヤー矩形の初回接触で回収する。</summary>
        /// <param name="center">プレイヤー中心座標。</param>
        /// <param name="size">プレイヤー矩形寸法。</param>
        void CheckPickup(math::Vector2f center, math::Vector2f size);
        /// <summary>地点、帰還案内、残り防御回数を描く。</summary>
        /// <param name="hudPanel">地点マーカーが文字を覆わないようにする画面領域。</param>
        void DrawExploration(const shape::Rect& hudPanel) const;
        /// <summary>防衛の効果を検証するため残HPを返す。</summary>
        /// <returns>現在のHP。</returns>
        float Health() const { return m_health; }
        /// @brief 修理と表示が共有する上限を返す。
        /// @return 依頼補正込みの最大HP。
        float MaxHealth() const { return m_maxHealth; }
        /// <summary>開始時に読み込んだ拠点中心Xを返す。</summary>
        /// <returns>開始X（u）。</returns>
        float StartX() const { return m_config.BaseCenter.x; }
        /// <summary>候補の接近時間を見積もるために前進速度を返す。</summary>
        /// <returns>基準設定の速度（u/s）。</returns>
        float Speed() const { return m_config.BaseSpeed; }
        /// <summary>開始時に読み込んだ終端Xを返す。</summary>
        /// <returns>終端X（u）。</returns>
        float EndX() const { return m_config.TrialEndX; }
        /// <summary>表示領域と画面外マーカーの計算に使う寸法を返す。</summary>
        /// <returns>拠点の幅・高さ（u）。</returns>
        math::Vector2f Size() const { return m_config.BaseSize; }
    private:
        shape::Rect m_explorationSite{{0, 0}, {32, 32}};
        bool m_collected{};
        bool m_delivered{};
        int m_shields{};
        component::CombatEffect m_hurtEffect;
        const float m_maxHealth; // この試行の修理上限。
        float m_health;
        stage::StageConfig m_config{}; // Phase 1試作設定（u / u/s）
        col2d::ColliderID m_id{};
    };
}
