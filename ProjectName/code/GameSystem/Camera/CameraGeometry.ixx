module;
#include <algorithm>
#include <cmath>

export module GameSystem.CameraGeometry;
import MyLib.Math.Vector2;

export namespace cameraGeometry
{
    // Same world-to-screen offset contract as Camera; a small world stays at origin.
    inline math::Vector2f ClampOffset(math::Vector2f offset, math::Vector2f world,
        math::Vector2f viewport)
    {
        return {std::clamp(offset.x, 0.0f, (std::max)(0.0f, world.x - viewport.x)),
                std::clamp(offset.y, 0.0f, (std::max)(0.0f, world.y - viewport.y))};
    }

    struct Marker { bool offscreen; math::Vector2f position; };

    inline Marker BaseMarker(math::Vector2f screenCenter, math::Vector2f baseHalfSize,
        math::Vector2f viewport)
    {
        // Keep the real rectangle until it is entirely outside the viewport.
        const bool outside = screenCenter.x + baseHalfSize.x <= 0 ||
            screenCenter.y + baseHalfSize.y <= 0 ||
            screenCenter.x - baseHalfSize.x >= viewport.x ||
            screenCenter.y - baseHalfSize.y >= viewport.y;
        const auto half = viewport.Half();
        const auto ray = screenCenter - half;
        // Intersect a center-to-target ray with an inset viewport. Space is reserved
        // for the arrow and label; scale >= 1 also handles a zero-length ray safely.
        const math::Vector2f extent{(std::max)(1.0f, half.x - 48.0f),
                                   (std::max)(1.0f, half.y - 48.0f)};
        const float scale = (std::max)({1.0f, std::abs(ray.x) / extent.x,
                                            std::abs(ray.y) / extent.y});
        return {outside, half + ray / scale};
    }
}
