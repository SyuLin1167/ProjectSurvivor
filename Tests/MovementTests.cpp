#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <json.hpp>

import Component.MovementMath;
import Phase1.Config;
import GameSystem.CameraGeometry;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;

/// <summary>回帰条件に失敗した場合は理由を表示してテストを終了する。</summary>
/// <param name="condition">成立を期待する検証条件。</param>
/// <param name="message">条件不成立時に表示する理由。</param>
void require(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

/// <summary>既存処理を用いて、このテストが担当する回帰条件を検証する。</summary>
/// <returns>全条件が成功すると0。不成立時はrequireから終了コード1で終了する。</returns>
int main()
{
    // 矩形判定は基盤Rectの接触込み契約を使用し、長距離でもマーカーの余白を保つ。
    const shape::Rect viewport{{0, 0}, {1920, 1080}};
    require(!cameraGeometry::BaseMarker({{-95, 508}, {96, 64}}, viewport).offscreen,
        "Partially visible base keeps its real rectangle");
    require(!cameraGeometry::BaseMarker({{-96, 508}, {96, 64}}, viewport).offscreen,
        "Rect touching viewport edge remains visible");
    require(cameraGeometry::BaseMarker({{-97, 508}, {96, 64}}, viewport).offscreen,
        "Disjoint rectangle shows indicator");
    for (auto center : {math::Vector2f{-16000, 540}, {16000, 540}, {960, -4000}, {960, 4000}, {16000, 4000}})
    {
        const auto marker = cameraGeometry::BaseMarker({center - math::Vector2f{48, 32}, {96, 64}}, viewport);
        require(marker.offscreen && marker.position.x >= 47.99f && marker.position.x <= 1872.01f &&
            marker.position.y >= 47.99f && marker.position.y <= 1032.01f, "Long-distance marker stays inset");
    }
    require(!cameraGeometry::BaseMarker({{912, 508}, {96, 64}}, viewport).offscreen,
        "Center / reunion has no offscreen marker");

    // ゲームと同じJSONと変換関数で承認初期値と不正データの拒否を確認する。
    std::ifstream input("ProjectName/resources/data/Phase1Config.json");
    require(input.is_open(), "Open config from repository root");
    const auto data = nlohmann::json::parse(input);
    const auto config = phase1::Config::FromJson(data);
    require(config.PlayerSpeed == 240 && config.PlayerSize == 32 &&
        config.WorldWidth == 16384 && config.WorldHeight == 4096 &&
        config.PlayerCenterX == 512 && config.PlayerCenterY == 2048 && config.GridSize == 128,
        "Approved DEC-01 initial values");
    const auto rejected = [](const nlohmann::json& invalid) {
        try { phase1::Config::FromJson(invalid); }
        catch (const std::exception&) { return true; }
        return false;
    };
    auto invalid = data; invalid.erase("playerSpeed");
    require(rejected(invalid), "Missing field");
    invalid = data; invalid["playerSpeed"] = "fast";
    require(rejected(invalid), "Wrong field type");
    invalid = data; invalid["gridSize"] = 0;
    require(rejected(invalid), "Zero grid spacing");
    invalid = data; invalid["gridSize"] = 1.5;
    require(rejected(invalid), "Fractional grid spacing");
    invalid = data; invalid["playerCenterX"] = 0;
    require(rejected(invalid), "Initial rectangle outside world");
    invalid = data; invalid["worldHeight"] = -1;
    require(rejected(invalid), "Negative world size");
    auto adjusted = data; adjusted["playerSpeed"] = 200;
    require(phase1::Config::FromJson(adjusted).PlayerSpeed == 200, "Read changed JSON value");

    // 全16組合せで逆方向相殺、無入力、方向の符号と単位長への正規化を確認する。
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

    // 同じ2秒の距離で斜め加速とフレーム時間の二重適用を検出する。
    for (int fps : { 30, 60, 120 })
    {
        for (auto v : { movement::ResolveDirection(false, true, false, false, true),
                        movement::ResolveDirection(false, true, false, true, true) })
        {
            float x = 512, y = 2048;
            for (int i = 0; i < fps * 2; ++i)
            {
                x += movement::BoundedDisplacement(x, v.x * config.PlayerSpeed / fps,
                    config.PlayerSize, config.WorldWidth);
                y += movement::BoundedDisplacement(y, v.y * config.PlayerSpeed / fps,
                    config.PlayerSize, config.WorldHeight);
            }
            require(std::abs(std::hypot(x - 512, y - 2048) - 480.0f) < 0.1f,
                "Equal-time cardinal / diagonal distance");
        }
    }

    // 長いフレームでも四辺を越えず、境界から離れられることを確認する。
    for (float extent : { config.WorldWidth, config.WorldHeight })
    {
        const float max = extent - config.PlayerSize;
        require(16 + movement::BoundedDisplacement(16, -100000, 32, extent) == 0, "Low edge");
        require(16 + movement::BoundedDisplacement(16, 100000, 32, extent) == max, "High edge");
        require(movement::BoundedDisplacement(0, -4, 32, extent) == 0, "Low edge hold");
        require(movement::BoundedDisplacement(max, 4, 32, extent) == 0, "High edge hold");
        require(movement::BoundedDisplacement(0, 4, 32, extent) == 4, "Leave low edge");
        require(movement::BoundedDisplacement(max, -4, 32, extent) == -4, "Leave high edge");
    }
    // 一方の軸が境界で止まっても、他方は進めることを確認する。
    float x = 0, y = 100;
    x += movement::BoundedDisplacement(x, -4, 32, config.WorldWidth);
    y += movement::BoundedDisplacement(y, 4, 32, config.WorldHeight);
    require(x == 0 && y == 104, "Slide along edge");
    require(movement::BoundedDisplacement(y, 0, 32, config.WorldHeight) == 0, "Stop");
    const auto legacy = movement::ResolveDirection(true, false, false, false, false);
    require(legacy.x == -1 && legacy.y == 0, "Horizontal-only sample");
    std::cout << "PASS: JSON config validation, 16 input combinations, 30/60/120fps distances, bounds, slide, stop, legacy\n";
}
