module;
#include <algorithm>
#include <cmath>

export module GameSystem.CameraGeometry;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;

export namespace cameraGeometry
{
    /// <summary>画面外判定と、矢印を置く画面内座標。</summary>
    struct Marker { bool offscreen; math::Vector2f position; };

    /// <summary>画面座標の拠点矩形から、画面端に余白を確保したマーカーを求める。</summary>
    /// <param name="base">画面座標の拠点矩形。</param>
    /// <param name="viewport">表示領域の矩形（px）。</param>
    /// <returns>画面外フラグと、余白内に収めたマーカーの画面座標。</returns>
    inline Marker BaseMarker(const shape::Rect& base, const shape::Rect& viewport)
    {
        // Rectの契約に合わせ、辺が接する場合も表示対象に含める。
        const bool outside = !base.AABB(viewport);
        const auto center = viewport.Center();
        const auto ray = base.Center() - center;
        // 画面中心から拠点への半直線を、矢印と文字用に48px縮めた矩形へ収める。
        // 軸別clampだけでは斜め方向が変わるため、同じ倍率で両軸を縮める。
        const auto half = viewport.size.Half();
        const math::Vector2f extent{(std::max)(1.0f, half.x - 48.0f),
                                   (std::max)(1.0f, half.y - 48.0f)};
        const float scale = (std::max)({1.0f, std::abs(ray.x) / extent.x,
                                            std::abs(ray.y) / extent.y});
        return {outside, center + ray / scale};
    }
}
