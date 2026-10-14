export module ObjectTag;

import <string>;
import <array>;
import <unordered_map>;
import <string>;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    /// <summary>
    /// オブジェクトタグ
    /// </summary>
    export enum ObjectTag :uint32_t
    {
        NONE = 0,
        MAP = 1 << 0,
        PLAYER = 1 << 1,
        ENEMY = 1 << 2,
        GOAL = 1 << 3,
    };

    /// <summary>
    /// タグ群
    /// </summary>
    export constexpr std::array OBJECT_TAGS
    {
        NONE,
        MAP,
        PLAYER,
        ENEMY,
        GOAL,
    };

    /// <summary>
    /// タグから文字列への変換
    /// </summary>
    export const std::unordered_map<uint32_t, std::string> TagToString
    {
        {ObjectTag::NONE,"none"},
        {ObjectTag::MAP,"map"},
        {ObjectTag::PLAYER,"player"},
        {ObjectTag::ENEMY,"enemy"},
        {ObjectTag::GOAL,"goal"},
    };

    /// <summary>
    /// 文字列からタグへの変換
    /// </summary>
    export const std::unordered_map<std::string, uint32_t> StringToTag
    {
        {"none",ObjectTag::NONE},
        {"map",ObjectTag::MAP},
        {"player",ObjectTag::PLAYER},
        {"enemy",ObjectTag::ENEMY},
        {"goal",ObjectTag::GOAL},
    };
}
