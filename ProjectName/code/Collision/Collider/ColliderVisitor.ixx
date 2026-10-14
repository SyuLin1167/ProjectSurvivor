export module ColliderVisitor;
import ColliderDef;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    // 前方宣言
    export class RectCollider;
    export class CircleCollider;

    /// <summary>
    /// コライダービジター
    /// </summary>
    export class ColliderVisitor
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        ColliderVisitor() = default;
        /// <summary>
        /// デストラクタ
        /// </summary>
        virtual ~ColliderVisitor() = default;

        /// <summary>
        /// 矩形コライダーへ訪問
        /// </summary>
        virtual void Visit(RectCollider& target) {};

        /// <summary>
        /// 円形コライダーへ訪問
        /// </summary>
        virtual void Visit(CircleCollider& target) {};

        /// <summary>
        /// 衝突後の共通処理
        /// </summary>
        ///　<typeparam name="Issue">衝突を起こしたコライダーの型</typeparam>
        ///　<typeparam name="Target">衝突相手のコライダーの型</typeparam>
        ///　<typeparam name="Resolve">解決処理の型</typeparam>
        template <class Issue, class Target, class Resolve>
        bool HandleContact(Issue& issue, Target& target, Resolve& resolve)
        {
            m_hadContact = true;

            // 自身が静的なら押し戻しは行わない（ループ終了）
            if (issue.GetColliderDef()->type == Type::STATIC)
            {
                return false;
            }

            // どちらかがトリガーなら押し戻しは行わない
            if (!issue.GetColliderDef()->isTrigger || !target.GetColliderDef()->isTrigger)
            {
                if (auto info = resolve.Resolve(issue, target))
                {
                    m_info = info.value();
                }
            }

            return true;
        }

    protected:
        bool m_hadContact = false;
        ContactInfo m_info; // 衝突情報
    };
}
