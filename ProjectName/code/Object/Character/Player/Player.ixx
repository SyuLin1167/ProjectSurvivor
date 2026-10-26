export module Object.Player;

import Phase1.Config;
import Object.PlayerAttackConfig;

import <future>;

import Object.GameObject;
import Object.PlayerData;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    /// <summary>
    /// プレイヤーの役割
    /// </summary>
    export class Player final :public GameObject
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="prototype">trueならPhase 1の見下ろし試作設定と移動を使用する。</param>
        explicit Player(bool prototype = false);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Player();

        /// <summary>
        /// 初期化処理
        /// </summary>
        void Init() override;
        /// <summary>攻撃時に現存する敵を取得する関数を保持する。</summary>
        /// <param name="referenceObj">所有するObjectManagerの取得関数。シーン内だけで使用する。</param>
        void GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj) override;

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

        /// <summary>LateUpdate後の中心座標を表示用に返す。移動座標は変更しない。</summary>
        /// <returns>プレイヤー矩形のワールド中心座標（u）。</returns>
        math::Vector2f Center() const { return m_pos + pData.size.Half(); }

        /// <summary>
        /// 自身のタグ
        /// </summary>
        /// <returns>プレイヤー</returns>
        ObjectTag MyObjectTag() const override
        {
            return ObjectTag::PLAYER;
        }

        /// <summary>
        /// プレイヤーのアセット名
        /// </summary>
        const struct AssetName
        {
            static constexpr const char* IDLE = "PlayerIdle"; // アイドル
            static constexpr const char* WALK = "PlayerWALK"; // 歩行
        };

        PlayerData pData; // プレイヤー情報
        col2d::ColliderID id;   // コライダーの識別子
    private:
        /// <summary>1更新につき最大1体へ、承認済みの自動攻撃を適用する。</summary>
        void UpdateAutoAttack();
        phase2::PlayerAttackConfig m_attack{};
        double m_attackRemaining{}; // 秒。初回と対象なしでは即時発動可能。
        std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> m_referenceObjects;
        std::weak_ptr<GameObject> m_lastHit; // 表示用。撃破した敵の寿命を延ばさない。
        math::Vector2f m_lastHitCenter{};
        float m_hitDisplayRemaining{};
        bool m_prototype; // Phase 1試作経路の選択
        phase1::Config m_phase1Config{}; // JSONから読み込んだ試作設定
        math::Vector2f m_cameraCenter{};
    };
}
