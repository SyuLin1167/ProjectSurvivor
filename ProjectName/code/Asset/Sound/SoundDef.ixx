export module Asset.SoundDef;

import <string>;

/// <summary>
/// アセット関連
/// </summary>
export namespace asset
{
    export constexpr int8_t SOUNDTYPE_BGM_LOOP = 0x0001;    // BGM(ループ)
    export constexpr int8_t SOUNDTYPE_SE_LOOP = 0x0002;     // SE(ループ)
    export constexpr int8_t SOUNDTYPE_BGM_ONCE = 0x0004;    // BGM
    export constexpr int8_t SOUNDTYPE_SE_ONCE = 0x0008;     // SE

    /// <summary>
    /// サウンドの状態
    /// </summary>
    export const enum class SoundState
    {
        PLAY,       // 再生中
        STOP,       // 停止中
        PAUSE,      // 一時停止中
    };

    /// <summary>
    /// サウンド定義
    /// </summary>
    export struct SoundDef final
    {
        SoundDef() = default;
        ~SoundDef() = default;

        int8_t soundType;        // サウンドの種類
        int8_t playType;        // 再生タイプ
        std::string name;       // サウンド名
        int volume;             // 個別ボリューム
    };
}