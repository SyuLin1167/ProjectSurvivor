export module Object.Player;

import Phase1.Config;

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
        explicit Player(bool prototype = false);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Player();

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
        bool m_prototype; // Phase 1試作経路の選択
        phase1::Config m_phase1Config{}; // JSONから読み込んだ試作設定
        math::Vector2f m_cameraCenter{};
    };
}
