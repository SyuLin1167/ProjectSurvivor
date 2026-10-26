export module Object.Enemy;

import Object.GameObject;
import Object.MovingBase;
import Object.EnemyConfig;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    constexpr float MAX_HEALTH = 1.0f;  // 最大体力

    /// <summary>
    /// 敵の役割
    /// </summary>
    export class Enemy final :public GameObject
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Enemy();

        /// <summary>Phase 2の固定配置用追跡敵を生成する。</summary>
        /// <param name="center">円中心のワールド初期座標（u）。</param>
        /// <param name="config">EnemyConfig::FromJsonで検証した敵設定。</param>
        Enemy(math::Vector2f center, const phase2::EnemyConfig& config);

        /// <summary>キャラバンへの弱参照を取得する。</summary>
        /// <param name="referenceObj">タグに一致するオブジェクト一覧の取得関数。</param>
        void GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj) override;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Enemy();

        /// <summary>
        /// 初期化処理
        /// </summary>
        void Init() override;

        /// <summary>
        /// 更新処理
        /// </summary>
        void Update() override;

        /// <summary>
        /// 後更新処理
        /// </summary>
        void LateUpdate() override;

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw() override;

        /// <summary>
        /// 自身のタグ
        /// </summary>
        /// <returns>敵</returns>
        ObjectTag MyObjectTag() const override
        {
            return ObjectTag::ENEMY;
        }

        /// <summary>生存・有効Colliderを持つ試作敵か確認する。</summary>
        /// <returns>自動攻撃の対象にできる場合true。</returns>
        bool CanBeAttacked() const;
        /// <summary>即時命中によるダメージを適用し、HP0なら衝突判定を無効化する。</summary>
        /// <param name="damage">正の有限ダメージ。</param>
        /// <returns>有効な対象へダメージを適用した場合true。</returns>
        bool TakeDamage(float damage);
        /// <summary>敵円の中心座標を返す。</summary>
        /// <returns>ワールド中心(u)。</returns>
        math::Vector2f Center() const { return m_pos; }
        /// <summary>同距離時の順序に使う世代付きCollider由来の個体IDを返す。</summary>
        /// <returns>初期化後から破棄まで変わらないID。初期化前は使用しない。</returns>
        uint64_t TargetId() const { return (uint64_t(id.index) << 16) | id.generation; }
        /// <summary>検証用の残HPを返す。</summary>
        /// <returns>残HP。</returns>
        float Health() const { return m_health; }
    private:
        /// <summary>全Colliderの移動後に接触を1回評価し、敵固有の間隔で拠点へ被害を与える。</summary>
        void UpdateContactAttack();
        double m_contactRemaining{}; // 秒。初回は即時、離脱しても残り時間は維持。
        bool m_wasTouching{}; // 再接触時に非接触時間の端数を次の攻撃へ繰り越さない。
        static constexpr float CONTACT_DAMAGE = 10.0f; // DEC-02固定値。
        static constexpr double CONTACT_INTERVAL = 1.0; // DEC-02固定値（秒）。
        bool m_prototype = false;
        float m_radius = 16.0f; // 旧サンプルの半径。試作ではJSON値を使用する。
        float m_speed{}; // u/s。試作の設定読み込み時に確定する。
        std::weak_ptr<MovingBase> m_base; // 所有しない。対象破棄後は追跡を停止する。
        col2d::ColliderID id;   // コライダーID
        float m_health;        // 体力
        float m_damage;        // ダメージ
    };
}
