export module Collider.RectColliderResolver;
import <optional>;
import ColliderDef;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    // 前方宣言
    class RectCollider;
    class CircleCollider;

    /// <summary>
    /// 矩形コライダーの解決処理
    /// </summary>
    /// <remarks>
    /// 矩形コライダーの衝突解決を行う
    /// </remarks>
    export class RectColliderResolver final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        RectColliderResolver() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~RectColliderResolver() = default;

        /// <summary>
        /// 解決処理
        /// </summary>
        /// <param name="issue">解決対象の矩形コライダー</param>
        /// <param name="other">他の矩形コライダー</param>
        std::optional<ContactInfo> Resolve(RectCollider& issue, const RectCollider& other);

        /// <summary>
        /// 解決処理
        /// </summary>
        /// <param name="issue">解決対象の矩形コライダー</param>
        /// <param name="other">他の円形コライダー</param>
        std::optional<ContactInfo> Resolve(RectCollider& issue, const CircleCollider& other);
    };
}
