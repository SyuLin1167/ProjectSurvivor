module;
#include <algorithm>

export module Component.MovementMath;
import MyLib.Math.Vector2;

export namespace movement
{
    /// <summary>逆方向を軸ごとに相殺し、必要なら正規化して斜めの加速を防ぐ。</summary>
    inline math::Vector2f ResolveDirection(bool left, bool right, bool up, bool down,
        bool normalize)
    {
        const math::Vector2f d{ float(int(right) - int(left)), float(int(down) - int(up)) };
        return normalize ? d.Norm() : d;
    }

    /// <summary>矩形全体がワールドに収まる移動量を返す。座標への反映は呼び出し側で一度だけ行う。</summary>
    inline float BoundedDisplacement(float position, float displacement,
        float size, float worldSize)
    {
        return std::clamp(position + displacement, 0.0f,
            (std::max)(0.0f, worldSize - size)) - position;
    }
}
