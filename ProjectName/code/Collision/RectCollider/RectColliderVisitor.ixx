export module Collider.RectColliderVisitor;
import ColliderVisitor;
import Collider.RectColliderResolver;
import MyLib.Math.Vector2;

using namespace math;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    // 前方宣言
    class RectCollider;
    class CircleCollider;

    /// <summary>
    /// 矩形コライダービジター
    /// </summary>
    /// <remarks>
    export class RectColliderVisitor final : public ColliderVisitor
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        RectColliderVisitor(RectCollider& issue);

        // デフォルトコンストラクタは削除
        RectColliderVisitor() = delete;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~RectColliderVisitor() = default;

        /// <summary>
        /// 矩形コライダーを訪問
        /// </summary>
        /// <param name="_target">訪問する矩形コライダー</param>
        void Visit(RectCollider& target) override;

        /// <summary>
        /// 円形コライダーを訪問
        /// </summary>
        /// <param name="_target">訪問する円形コライダー</param>
        void Visit(CircleCollider& target) override;

    private:
        RectCollider& m_issue; // 訪問する矩形コライダーの参照
        RectColliderResolver m_resolver; // 矩形コライダーの解決処理を行うリゾルバー
    };
}
