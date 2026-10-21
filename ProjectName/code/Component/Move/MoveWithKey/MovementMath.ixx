module;
#include <algorithm>

export module Component.MovementMath;
import MyLib.Math.Vector2;

export namespace movement
{
    /// <summary>逆方向を軸ごとに相殺し、必要なら正規化して斜めの加速を防ぐ。</summary>
    /// <param name="left">左方向の入力が押されているか。</param>
    /// <param name="right">右方向の入力が押されているか。</param>
    /// <param name="up">上方向の入力が押されているか。</param>
    /// <param name="down">下方向の入力が押されているか。</param>
    /// <param name="normalize">trueなら非ゼロの入力方向を正規化する。</param>
    /// <returns>入力方向ベクトル。無入力・相殺時はゼロ。</returns>
    inline math::Vector2f ResolveDirection(bool left, bool right, bool up, bool down,
        bool normalize)
    {
        const math::Vector2f d{ float(int(right) - int(left)), float(int(down) - int(up)) };
        return normalize ? d.Norm() : d;
    }

    /// <summary>矩形全体がワールドに収まる移動量を返す。座標への反映は呼び出し側で一度だけ行う。</summary>
    /// <param name="position">対象軸の矩形左上座標（u）。</param>
    /// <param name="displacement">対象軸の要求移動量（u）。</param>
    /// <param name="size">対象軸の矩形サイズ（u）。</param>
    /// <param name="worldSize">対象軸のワールドサイズ（u）。</param>
    /// <returns>ワールド境界で制限した移動量（u）。</returns>
    inline float BoundedDisplacement(float position, float displacement,
        float size, float worldSize)
    {
        return std::clamp(position + displacement, 0.0f,
            (std::max)(0.0f, worldSize - size)) - position;
    }
}
