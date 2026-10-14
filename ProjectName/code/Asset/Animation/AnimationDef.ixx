export module Asset.AnimationDef;

import <string>;
import MyLib.Math.Vector2;

/// <summary>
/// アセット関連
/// </summary>
export namespace asset
{
    /// <summary>
    /// アニメーションの種類
    /// </summary>
    export const enum class AnimType
    {
        LOOP,       // ループ再生
        ONCE,       // 一度だけ再生
        PINGPONG    // 行きと帰りで再生
    };

    /// <summary>
    /// アニメーションの状態
    /// </summary>
    export const enum class AnimState
    {
        PLAY,       // 再生中
        STOP,       // 停止中
        PAUSE,      // 一時停止中
    };

    /// <summary>
    /// アニメーション定義
    /// </summary>
    export struct AnimationDef final
    {
        AnimationDef() = default;
        ~AnimationDef() = default;

        AnimType type = AnimType::LOOP; // アニメーションの種類
        std::string name; // アニメーション名
        float startFrame = 1; // 開始フレーム
        float endFrame; //終了フレーム
        float animationSpeed; // アニメーション速度
        math::Vector2f size; // サイズ
    };
}