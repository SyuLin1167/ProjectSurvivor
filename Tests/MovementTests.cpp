#include "../ProjectName/code/Component/Move/MoveWithKey/MovementMath.h"
#include "../ProjectName/code/Phase1Config.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

void require(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

int main()
{
    // All simultaneous direction combinations, including cancellation and idle.
    for (int bits = 0; bits < 16; ++bits)
    {
        const bool l = bits & 1, r = bits & 2, u = bits & 4, d = bits & 8;
        const auto v = movement::ResolveDirection(l, r, u, d, true);
        const float length = std::hypot(v.x, v.y);
        require(std::abs(length - ((l != r || u != d) ? 1.0f : 0.0f)) < 1e-6f,
            "Direction length / opposite input cancellation");
        require((v.x < 0) == (l && !r) && (v.x > 0) == (r && !l), "X direction");
        require((v.y < 0) == (u && !d) && (v.y > 0) == (d && !u), "Y direction");
    }

    for (int fps : { 30, 60, 120 })
    {
        for (auto v : { movement::ResolveDirection(false, true, false, false, true),
                        movement::ResolveDirection(false, true, false, true, true) })
        {
            float x = 512, y = 2048;
            for (int i = 0; i < fps * 2; ++i)
            {
                x += movement::BoundedDisplacement(x, v.x * phase1::PlayerSpeed / fps,
                    phase1::PlayerSize, phase1::WorldWidth);
                y += movement::BoundedDisplacement(y, v.y * phase1::PlayerSpeed / fps,
                    phase1::PlayerSize, phase1::WorldHeight);
            }
            require(std::abs(std::hypot(x - 512, y - 2048) - 480.0f) < 0.1f,
                "Equal-time cardinal / diagonal distance");
        }
    }

    // Overshoot all boundaries, including an unusually long frame.
    for (float extent : { phase1::WorldWidth, phase1::WorldHeight })
    {
        const float max = extent - phase1::PlayerSize;
        require(16 + movement::BoundedDisplacement(16, -100000, 32, extent) == 0, "Low edge");
        require(16 + movement::BoundedDisplacement(16, 100000, 32, extent) == max, "High edge");
        require(movement::BoundedDisplacement(0, -4, 32, extent) == 0, "Low edge hold");
        require(movement::BoundedDisplacement(max, 4, 32, extent) == 0, "High edge hold");
        require(movement::BoundedDisplacement(0, 4, 32, extent) == 4, "Leave low edge");
        require(movement::BoundedDisplacement(max, -4, 32, extent) == -4, "Leave high edge");
    }
    float x = 0, y = 100;
    x += movement::BoundedDisplacement(x, -4, 32, phase1::WorldWidth);
    y += movement::BoundedDisplacement(y, 4, 32, phase1::WorldHeight);
    require(x == 0 && y == 104, "Slide along edge");
    require(movement::BoundedDisplacement(y, 0, 32, phase1::WorldHeight) == 0, "Stop");
    const auto legacy = movement::ResolveDirection(true, false, false, false, false);
    require(legacy.x == -1 && legacy.y == 0, "Horizontal-only sample");
    std::cout << "PASS: 16 input combinations, 30/60/120fps distances, bounds, slide, stop, legacy\n";
}
