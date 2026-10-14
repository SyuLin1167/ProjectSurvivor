export module ObjectState;
import <cstdint>;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    /// <summary>
    /// メインステータス
    /// </summary>
    export enum MainState :uint32_t
    {
        ACTIVE = 0,           //アクティブ状態
        PAUSE = 1 << 0,       //停止状態
        DEAD = 1 << 1,        //死亡状態
    };
}
