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
    const int UP = KEY_INPUT_UP; // UP キー（Phase 1の移動入力）
    const int DOWN = KEY_INPUT_DOWN; // DOWN キー（Phase 1の移動入力）
    const int W = KEY_INPUT_W; // W キー（Phase 1の移動入力）
    const int A = KEY_INPUT_A; // A キー（Phase 1の移動入力）
    const int S = KEY_INPUT_S; // S キー（Phase 1の移動入力）
    const int D = KEY_INPUT_D; // D キー（Phase 1の移動入力）
}keyType;


/// <summary>
/// 状態更新の対象キー一覧。KeyTypeの追加だけでなく、ここへの登録も必要
/// </summary>
export const std::vector<int> KEYS =
{
    keyType.ESCAPE,
    keyType.E,
    keyType.R,
    keyType.SPACE,
    keyType.LEFT,
    keyType.RIGHT,
    keyType.UP, keyType.DOWN,
    keyType.W, keyType.A, keyType.S, keyType.D,
};
