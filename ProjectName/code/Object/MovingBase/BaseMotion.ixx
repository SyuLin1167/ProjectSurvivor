module;
#include <algorithm>

export module Object.BaseMotion;
import MyLib.Math.Vector2;

export namespace phase1
{
    // Phase 1の方向決定だけを担当する。将来の経路判断はここから分離して置換できる。
    inline math::Vector2f BaseDirection() { return {1.0f, 0.0f}; }

    // 既存Colliderの「速度」は1フレームの変位。秒への換算はここで一度だけ行う。
    // 入力・プレイヤー座標・カメラを受け取らないため、離脱や画面外でも同じ計算になる。
    inline math::Vector2f BaseDisplacement(float centerX, float endX, float speed, float dt)
    {
        const float remaining = (std::max)(0.0f, endX - centerX);
        return BaseDirection() * (std::min)(remaining, speed * (std::max)(0.0f, dt));
    }
}
