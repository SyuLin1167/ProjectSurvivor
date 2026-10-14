module;
#include <DxLib.h>

export module MyLib.KeyStatus.KeyTag;

import <vector>;
import <memory>;

/// <summary>
/// ゲームの操作に必要なキー
/// </summary>
export constexpr struct KeyType
{
    const int ESCAPE = KEY_INPUT_ESCAPE;    //エスケープキー
    const int E = KEY_INPUT_E;              //Eキー
    const int R = KEY_INPUT_R;              //Rキー
    const int SPACE = KEY_INPUT_SPACE;      //SPACEキー
    const int LEFT = KEY_INPUT_LEFT;        //左キー
    const int RIGHT = KEY_INPUT_RIGHT;      //右キー
}keyType;


/// <summary>
/// ループ制御用キー一覧
/// </summary>
export const std::vector<int> KEYS =
{
    keyType.ESCAPE,
    keyType.E,
    keyType.R,
    keyType.SPACE,
    keyType.LEFT,
    keyType.RIGHT,
};