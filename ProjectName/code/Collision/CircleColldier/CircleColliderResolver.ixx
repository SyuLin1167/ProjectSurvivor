export module Collider.CircleColliderResolver;
import <optional>;
import MyLib.Math.Vector2;
import ColliderDef;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    class RectCollider;
    class CircleCollider;

    /// <summary>
    /// 円形コライダー解決
    /// </summary>
    export class CircleColliderResolver
    {
    public:
        CircleColliderResolver() = default;
        ~CircleColliderResolver() = default;

        /// <summary>
        /// 解決処理
        /// </summary>
        /// <param name="issue">解決対象の円形コライダー</param>
        /// <param name="other">他の矩形コライダー</param>
        std::optional<ContactInfo> Resolve(CircleCollider& issue, const RectCollider& other);

        /// <summary>
        /// 解決処理
        /// </summary>
        /// <param name="issue">解決対象の円形コライダー</param>
        /// <param name="other">他の円形コライダー</param>
        std::optional<ContactInfo> Resolve(CircleCollider& issue, const CircleCollider& other);
    };
}