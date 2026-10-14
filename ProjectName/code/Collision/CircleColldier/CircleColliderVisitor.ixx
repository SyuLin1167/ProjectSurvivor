export module Collider.CircleColliderVisitor;
import ColliderVisitor;
import Collider.CircleColliderResolver;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    // 前方宣言
    class CircleCollider;
    class RectCollider;

    /// <summary>
    /// 円形コライダービジター
    /// </summary>
    export class CircleColliderVisitor final : public ColliderVisitor
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        CircleColliderVisitor(CircleCollider& issue);

        // デフォルトコンストラクタは削除
        CircleColliderVisitor() = delete;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~CircleColliderVisitor() = default;

        /// <summary>
        /// 矩形コライダーを訪問
        /// </summary>
        void Visit(RectCollider& target) override;

        /// <summary>
        /// 円形コライダーを訪問
        /// </summary>
        void Visit(CircleCollider& target) override;

    private:
        CircleCollider& m_issue;                 // 衝突判定を行う円形コライダー
        CircleColliderResolver m_resolver;       // 解決処理
    };
}