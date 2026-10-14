export module Collider.ColliderManager;

import <queue>;
import <vector>;
import <memory>;
import <functional>;
import <unordered_map>;
import <string>;
import <future>;

import MyLib.Math.Vector2;
import Collider;
import Collider.RectCollider;
import Collider.TileCollider;
import Collider.CircleCollider;
import Object.MapInfo;

using namespace math;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    /// <summary>
    /// コライダーの識別子
    /// </summary>
    export struct ColliderID
    {
        uint32_t index = 0xFFFFFFFF;    // インデックス(既定値は無効)
        uint16_t generation;     // 生成世代
    };

    /// <summary>
    /// コライダー管理
    /// </summary>
    export class ColliderManager final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        ColliderManager() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~ColliderManager() = default;

        /// <summary>
        /// コライダーを生成
        /// </summary>
        /// <typeparam name="T">コライダーの型</typeparam>
        /// <typeparam name="Args">コライダーの生成時に渡す引数の型</typeparam>
        /// <param name="def">コライダー定義</param>
        /// <param name="ownerID">所有者のID</param>
        /// <param name="args">コライダー生成時に渡す引数</param>
        /// <returns>生成されたコライダーの識別子</returns>
        template<typename T, typename... Args>
        [[nodiscard]] inline ColliderID CreateCollider(ColliderDef* def, const uint32_t& ownerID, Args&&... args)
        {
            auto col = std::make_unique<T>(def, std::forward<Args>(args)...);
            col->GenerateCategory(ownerID);
            ColliderID id = CreateID();
            m_colliders[id.index] = std::move(col);
            return id;
        }

        /// <summary>
        /// 矩形コライダーを生成
        /// </summary>
        /// <param name="def">コライダー定義</param>
        /// <param name="size">サイズ</param>
        /// <param name="ownerID">所有者のID</param>
        /// <returns>生成されたコライダーの識別子</returns>
        [[nodiscard]] inline ColliderID CreateRectCollider(ColliderDef* def, const Vector2f& size, const uint32_t& ownerID = 0)
        {
            return CreateCollider<RectCollider>(def, ownerID, size);
        }

        /// <summary>
        /// 円コライダーを生成
        /// </summary>
        /// <param name="def">コライダー定義</param>
        /// <param name="radius">半径</param>
        /// <param name="ownerID">所有者のID</param>
        /// <returns>生成されたコライダーの識別子</returns>
        [[nodiscard]] inline ColliderID CreateCircleCollider(ColliderDef* def, float radius, const uint32_t& ownerID = 0)
        {
            return CreateCollider<CircleCollider>(def, ownerID, radius);
        }

        /// <summary>
        /// タイルコライダーを生成
        /// </summary>
        /// <param name="def">コライダー定義</param>
        /// <param name="ownerID">所有者のID</param>
        /// <returns>生成されたコライダーの識別子</returns>
        [[nodiscard]] inline ColliderID CreateTileCollider(ColliderDef* def, const object::MapInfo& info, std::string fileName, const uint32_t& ownerID = 0)
        {
            return CreateCollider<TileCollider>(def, ownerID, info, fileName);
        }

        /// <summary>
        /// タイルコライダーを生成
        /// </summary>
        /// <param name="def">コライダー定義</param>
        /// <param name="ownerID">所有者のID</param>
        /// <returns>生成されたコライダーの識別子</returns>
        [[nodiscard]] inline ColliderID CreateTileCollider(ColliderDef* def, std::shared_future<object::MapInfo> info, std::string fileName, const uint32_t& ownerID = 0)
        {
            return CreateCollider<TileCollider>(def, ownerID, info, fileName);
        }

        /// <summary>
        /// コライダーを削除
        /// </summary>
        /// <param name="id">コライダーの識別子</param>
        void Destroy(const ColliderID& id);

        /// <summary>
        /// コライダーを取得
        /// </summary>
        /// <param name="id">コライダーの識別子</param>
        /// <returns>コライダーへのポインタ</returns>
        Collider* GetCollider(const ColliderID& id) const;

        /// <summary>
        /// コライダーの更新処理
        /// </summary>
        void Step();

        /// <summary>
        /// マスクを追加
        /// </summary>
        /// <param name="id">コライダーの識別子</param>
        /// <param name="shape">形状の種類</param>
        /// <param name="ownerID">所有者のID</param>
        void AddMask(const ColliderID& id, const uint32_t& shape, const uint32_t& ownerID)
        {
            // 世代が一致しない(古い/無効な)IDは無視する
            if (Collider* collider = GetCollider(id))
            {
                collider->GetFilter().AddMask(shape, ownerID);
            }
        }

        /// <summary>
        /// 衝突時イベントを追加
        /// </summary>
        /// <param name="id">コライダーの識別子</param>
        /// <param name="key">対象のイベントを識別するためのキー</param>
        /// <param name="event">追加するイベント</param>
        void AddEvent(const ColliderID& id, const uint64_t& key, ContactListener event)
        {
            // 世代が一致しない(古い/無効な)IDは無視する
            if (Collider* collider = GetCollider(id))
            {
                collider->AddEvent(key, event);
            }
        }

    private:
        /// <summary>
        /// コライダーの識別子を生成
        /// </summary>
        /// <returns>生成されたコライダーの識別子</returns>
        ColliderID CreateID();

        /// <summary>
        /// IDスロット
        /// </summary>
        /// <remarks>
        /// スロットは削除せず、vectorのサイズが単調増加する採番カウンターを兼ねる。
        /// generationは「次にこのindexへ割り当てる/現在有効な世代」を表す。
        /// Destroy時にのみ世代を進め、CreateIDでは進めない。
        /// </remarks>
        struct IDSlot
        {
            uint16_t generation = 0;    // 現在の世代
            bool alive = false;         // 使用中か
        };

        /// <summary>
        /// IDが現在有効か(範囲内・使用中・世代一致)
        /// </summary>
        bool IsValid(const ColliderID& id) const
        {
            return id.index < m_slots.size()
                && m_slots[id.index].alive
                && m_slots[id.index].generation == id.generation;
        }

        std::queue<uint32_t> m_freeIndex;   // 再利用可能なindex(重複登録しない)
        std::vector<IDSlot> m_slots;        // index毎の世代と使用状態
        std::unordered_map<uint32_t, std::unique_ptr<Collider>> m_colliders; // コライダーのマップ
    };
}
