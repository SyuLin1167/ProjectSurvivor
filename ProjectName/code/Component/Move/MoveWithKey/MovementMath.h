#pragma once
#include <algorithm>

namespace movement
{
    struct Direction { float x; float y; };

    inline Direction ResolveDirection(bool left, bool right, bool up, bool down,
        bool normalize)
    {
        Direction d{ float(int(right) - int(left)), float(int(down) - int(up)) };
        if (normalize && d.x != 0.0f && d.y != 0.0f)
        {
            constexpr float diagonal = 0.7071067811865475f;
            d.x *= diagonal;
            d.y *= diagonal;
        }
        return d;
    }

    // RectCollider positions are top-left, not center coordinates.
    inline float BoundedDisplacement(float position, float displacement,
        float size, float worldSize)
    {
        return std::clamp(position + displacement, 0.0f,
            (std::max)(0.0f, worldSize - size)) - position;
    }
}
