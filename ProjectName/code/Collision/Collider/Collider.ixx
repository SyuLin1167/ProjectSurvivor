export module Collider;

import <memory>;
import <functional>;
import <unordered_map>;

export import MyLib.Math.Vector2;
export import ColliderDef;
export import ColliderFilter;
export import ColliderVisitor;

using namespace math;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    constexpr float EPSILON = 1e-6f;    // 浮動小数点誤差許容値

    constexpr Vector2f NORMAL_LEFT = { -1.0f, 0 }; // 左方向の法線
    constexpr Vector2f NORMAL_RIGHT = { 1.0f, 0 }; // 右方向の法線
    constexpr Vector2f NORMAL_TOP = { 0, -1.0f }; // 上方向の法線
    constexpr Vector2f NORMAL_BOTTOM = { 0, 1.0f }; // 下方向の法線

    /// <summary>
    /// コンタクトリスナー
    /// </summary>
    export struct ContactListener
    {
        std::function<bool(const ContactInfo&)> when; // 条件
        std::function<void()> event; // 処理
    };

    /// <summary>
    /// コライダー
    /// </summary>
    export class Collider
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="def">コライダー定義</param>
        explicit Collider(ColliderDef* def)
            : m_colDef(std::make_unique<ColliderDef>(*def))
            , m_filter()
            , m_velocity{ 0,0 }
        {}

        /// <summary>
        /// デストラクタ
        /// </summary>
        virtual ~Collider();

        /// <summary>
        /// カテゴリーの生成
        /// </summary>
        /// <param name="ownerID">所有者のID</param>
        virtual void GenerateCategory(uint32_t ownerID = 0) = 0;

        /// <summary>
        /// コライダーの定義情報を取得
        /// </summary>
        /// <returns>コライダー定義情報へのポインタ</returns>
        ColliderDef* GetColliderDef() const noexcept
        {
            return m_colDef.get();
        }

        /// <summary>
        /// ビジターの受け入れ
        /// </summary>
        /// /// <param name="visitor">他のコライダービジター</param>
        virtual void Accept(ColliderVisitor& visitor) = 0;

        /// <summary>
        /// 衝突判定
        /// </summary>
        /// <param name="other">他のコライダー</param>
        virtual void CollideWith(Collider& other)
        {
            other.Accept(*m_visitor);
        }

        /// <summary>
        /// コライダーのフィルタ情報を取得
        /// </summary>
        Filter& GetFilter() noexcept
        {
            return m_filter;
        }

        /// <summary>
        /// 移動量の設定
        /// </summary>
        void SetVelocity(const Vector2f& velocity) noexcept
        {
            m_velocity = velocity;
        }

        /// <summary>
        /// 移動量の取得
        /// </summary>
        /// <returns>移動量</returns>
        Vector2f GetVelocity() const noexcept
        {
            return m_velocity;
        }

        /// <summary>
        /// 移動量の追加
        /// </summary>
        virtual void AddVelocity() {};

        /// <summary>
        /// イベントの追加
        /// </summary>
        /// <param name="key">イベントのキー</param>
        /// <param name="event">追加するイベント</param>
        void AddEvent(const uint64_t& key, ContactListener event)
        {
            m_events[key].push_back(std::move(event));
        }

        /// <summary>
        /// イベントの発火
        /// </summary>
        /// <param name="key">イベントのキー</param>
        /// <param name="info">衝突情報</param>
        void TriggerEvent(const uint64_t& key, const ContactInfo& info);

        /// <summary>
        /// イベントの削除
        /// </summary>
        /// <param name="key">イベントのキー</param>
        void DeleteEvent(const uint64_t& key)
        {
            if (m_events.find(key) != m_events.end())
            {
                m_events.erase(key);
            }
        }

    protected:
        /// <summary>
        /// 移動量の追加
        /// </summary>
        /// <param name="velocity">移動量</param>
        virtual void AddVelocity(const Vector2f& velocity) {};

        Filter m_filter; // コライダーのフィルタ情報
        std::unique_ptr<ColliderDef> m_colDef; // コライダー定義情報へのポインタ
        std::unique_ptr<ColliderVisitor> m_visitor; // ビジター
        Vector2f m_velocity; // 速度
        std::unordered_map<uint64_t, std::vector<ContactListener>> m_events; // 接触イベントリスト
    };
}
