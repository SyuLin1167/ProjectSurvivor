export module Collider.TileColliderResolver;
import <cstdint>;
import <optional>;
import Collider.RectCollider;
import Collider.TileCollider;
import MyLib.Shape.Rect;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    /// <summary>
    /// タイルコライダーの解決処理
    /// </summary>
    /// <remarks>
    /// タイルコライダーの衝突解決を行う
    /// </remarks>
    export class TileColliderResolver
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        TileColliderResolver() = default;
        /// <summary>
        /// デストラクタ
        /// </summary>
        ~TileColliderResolver() = default;

        /// <summary>
        /// 解決処理
        /// </summary>
        /// <param name="issue">解決対象のタイルコライダー</param>
        /// <param name="tileInfo">タイル情報</param>
        /// <returns>衝突情報</returns>
        std::optional<ContactInfo> Resolve(RectCollider& issue, const TileInfo& tileInfo);

        // 押し戻し量の計算
        math::Vector2f CalcPushBack(const RectCollider& issue,const uint8_t adjacentFlag, const shape::Rect& tileRect);

        /// <summary>
        /// 通常ブロックの解決処理
        /// </summary>
        /// <param name="issue">解決対象のタイルコライダー</param>
        /// <param name="tileInfo">タイル情報</param
        /// <returns>衝突情報</returns>
        std::optional<ContactInfo> ResolveSolid(RectCollider& issue, const TileInfo& tileInfo);

        /// <summary>
        /// 上からのみ当たるブロックの解決処理
        /// </summary>
        /// <param name="issue">解決対象のタイルコライダー</param>
        /// <param name="tileInfo">タイル情報</param>
        /// <returns>衝突情報</returns>
        std::optional<ContactInfo> ResolveOneWayTop(RectCollider& issue, const TileInfo& tileInfo);
    };
}

