export module Object.Enemy;

import Object.GameObject;

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
        void LateUpdate() override {};

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
        col2d::ColliderID id;   // コライダーID
        float m_health;        // 体力
        float m_damage;        // ダメージ
    };
}
