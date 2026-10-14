export module Collider.TileColliderVisitor;
export import ColliderVisitor;
import Collider.TileColliderResolver;
import Collider.RectCollider;
import Collider.TileCollider;

export namespace col2d
{
    /// <summary>
    /// タイルコライダービジター
    /// </summary>
    export class TileColliderVisitor final : public ColliderVisitor
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="issue">訪問するタイルコライダー</param>
        TileColliderVisitor(TileCollider& issue);

        // デフォルトコンストラクタは削除
        TileColliderVisitor() = delete;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~TileColliderVisitor() = default;

        /// <summary>
        /// タイルコライダーを訪問
        /// </summary>
        /// <param name="collider">訪問するタイルコライダー</param>
        void Visit(RectCollider& collider) override;

        /// <summary>
        /// 円形コライダーを訪問
        /// </summary>
        /// <param name="target">訪問する円形コライダー</param>
        void Visit(CircleCollider& target) override
        {
            // タイルコライダーは円形コライダーに対応していないため、処理なし
        }

        /// <summary>
        /// 衝突タイル毎の処理
        /// </summary>
        /// <param name="collider">矩形コライダー</param>
        /// <param name="axis">判定軸</param>
        void ProcessCollisionTiles(RectCollider& collider);

    private:
        TileCollider& m_issue;              // 訪問するタイルコライダーの参照
        TileColliderResolver m_resolver;    // タイルコライダーの解決処理を行うリゾルバー
    };
}
