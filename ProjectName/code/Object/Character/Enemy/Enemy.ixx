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

    private:
        bool m_prototype = false;
        float m_radius = 16.0f; // 旧サンプルの半径。試作ではJSON値を使用する。
        float m_speed{}; // u/s。試作の設定読み込み時に確定する。
        std::weak_ptr<MovingBase> m_base; // 所有しない。対象破棄後は追跡を停止する。
        col2d::ColliderID id;   // コライダーID
        float m_health;        // 体力
        float m_damage;        // ダメージ
    };
}
