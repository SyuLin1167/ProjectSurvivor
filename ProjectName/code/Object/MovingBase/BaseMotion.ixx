module;
#include <algorithm>

export module Object.BaseMotion;
import MyLib.Math.Vector2;

export namespace phase1
{
    /// <summary>Phase 1の直進方向を返し、方向決定を座標更新から分離する。</summary>
    /// <returns>+X方向の単位ベクトル。</returns>
    inline math::Vector2f BaseDirection() { return {1.0f, 0.0f}; }

    /// <summary>終端を越えない1フレームの変位を返す。速度(u/s)からの換算はここで一度だけ行う。</summary>
    // 入力・プレイヤー座標・カメラを受け取らないため、離脱や画面外でも同じ計算になる。
    /// <param name="centerX">現在の拠点中心X座標（u）。</param>
    /// <param name="endX">試行終了の中心X座標（u）。</param>
    /// <param name="speed">拠点の前進速度（u/s、非負）。</param>
    /// <param name="dt">経過秒数。負値は0として扱う。</param>
    /// <returns>終端までの残距離で制限した変位（u）。到達済みならゼロ。</returns>
    inline math::Vector2f BaseDisplacement(float centerX, float endX, float speed, float dt)
    {
        const float remaining = (std::max)(0.0f, endX - centerX);
        return BaseDirection() * (std::min)(remaining, speed * (std::max)(0.0f, dt));
    }
}
