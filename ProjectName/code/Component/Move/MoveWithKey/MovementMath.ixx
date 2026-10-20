module;
#include <algorithm>

export module Component.MovementMath;
import MyLib.Math.Vector2;

export namespace movement
{
    // 逆方向を軸ごとに相殺し、基盤の正規化で斜めの加速を防ぐ。
    inline math::Vector2f ResolveDirection(bool left, bool right, bool up, bool down,
        bool normalize)
    {
        const math::Vector2f d{ float(int(right) - int(left)), float(int(down) - int(up)) };
        return normalize ? d.Norm() : d;
    }

    // 左上座標から矩形全体が収まる移動量を返す。座標への反映は呼び出し側で一度だけ行う。
    inline float BoundedDisplacement(float position, float displacement,
        float size, float worldSize)
    {
        return std::clamp(position + displacement, 0.0f,
            (std::max)(0.0f, worldSize - size)) - position;
    }
}
