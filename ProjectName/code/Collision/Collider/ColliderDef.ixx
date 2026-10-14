export module ColliderDef;

import MyLib.Math.Vector2;

using namespace math;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    /// <summary>
    /// コライダーの種類
    /// </summary>
    export enum class Type
    {
        DYNAMIC,    // 動的
        STATIC      // 静的
    };

    /// <summary>
    /// コライダー定義
    /// </summary>
    /// <remarks>
    /// コライダーの定義情報を保持する
    /// </remarks>
    export struct ColliderDef final
    {
        /// <summary>
        /// コンストラクタ
        /// </summary>
        ColliderDef() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~ColliderDef() = default;

        Type type{ Type::DYNAMIC }; // コライダーの種類
        Vector2f worldPos{ 0,0 };   // ワールド座標
        Vector2f localPos{ 0,0 };   // ローカル座標
        bool shouldCCD;             // 連続衝突検出を行うか
        bool isTrigger;             // トリガーかどうか
        bool isActive;              // 有効かどうか
    };

    /// <summary>
    /// 衝突情報
    /// </summary>
    export struct ContactInfo
    {
        Vector2f normal{}; // 法線
        Vector2f point{}; // 衝突点
    };
}


