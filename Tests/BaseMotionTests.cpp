#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <json.hpp>

import Object.BaseMotion;
import Phase1.Config;

void require(bool condition, const char* message)
{
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}

int main()
{
    std::ifstream input("ProjectName/resources/data/Phase1Config.json");
    require(input.is_open(), "Open config from repository root");
    const auto data = nlohmann::json::parse(input);
    const auto c = phase1::Config::FromJson(data);
    require(c.BaseSpeed == 80 && c.BaseWidth == 96 && c.BaseHeight == 64 &&
        c.BaseCenterX == 768 && c.BaseCenterY == 2048 && c.TrialEndX == 15000,
        "Approved base defaults");

    // ゲームと同じ変位関数を各FPSで実行。ここでの加算はCollider::Stepの代わりであり、
    // このテストだけでは実際のObjectManager/Collider/描画・入力統合を保証しない。
    for (int fps : {30, 60, 120})
    {
        float x = c.BaseCenterX;
        for (int frame = 0; frame < fps * 60; ++frame)
        {
            const auto delta = phase1::BaseDisplacement(x, c.TrialEndX, c.BaseSpeed, 1.0f / fps);
            require(delta.y == 0 && delta.x > 0, "Continuous positive X movement");
            x += delta.x;
        }
        const float expected = c.BaseSpeed * 60;
        const float error = std::abs((x - c.BaseCenterX) - expected);
        std::cout << fps << "fps 60s: distance=" << x - c.BaseCenterX
                  << " expected=" << expected << " error=" << error << "u\n";
        require(error < 2.0f, "60s distance tolerance 2u");
    }

    // Pが10秒停止して拠点を先行させたあと追跡する数値シミュレーション。
    float base = c.BaseCenterX, player = c.PlayerCenterX;
    for (int i = 0; i < 600; ++i)
        base += phase1::BaseDisplacement(base, c.TrialEndX, c.BaseSpeed, 1.0f / 60).x;
    const float gap = base - player;
    int frames = 0;
    while (player < base && frames < 600)
    {
        base += phase1::BaseDisplacement(base, c.TrialEndX, c.BaseSpeed, 1.0f / 60).x;
        player += c.PlayerSpeed / 60;
        ++frames;
    }
    std::cout << "stop 10s gap=" << gap << "u, catch-up=" << frames / 60.0f
              << "s, expected=" << gap / (c.PlayerSpeed - c.BaseSpeed) << "s\n";
    require(player >= base && std::abs(frames / 60.0f - gap / (c.PlayerSpeed - c.BaseSpeed)) < 0.04f,
        "Catch-up using relative speed");

    // 長いフレームが終端をまたいでも、終端中心に一度だけ到達して以後は動かない。
    const auto last = phase1::BaseDisplacement(c.TrialEndX - 1, c.TrialEndX, c.BaseSpeed, 10);
    require(last.x == 1 && last.y == 0, "Clamp trial endpoint");
    require(phase1::BaseDisplacement(c.TrialEndX, c.TrialEndX, c.BaseSpeed, 1).x == 0, "Endpoint stop");
    require(phase1::BaseDisplacement(768, c.TrialEndX, c.BaseSpeed, 0).x == 0, "Zero delta time");

    // 同じデータ変換・移動関数でJSON変更の反映を実測（配布JSON自体は変更しない）。
    auto adjusted = data;
    adjusted["baseSpeed"] = 100; adjusted["playerSpeed"] = 200;
    const auto tune = phase1::Config::FromJson(adjusted);
    require(phase1::BaseDisplacement(tune.BaseCenterX, tune.TrialEndX, tune.BaseSpeed, 2).x == 200,
        "Changed JSON speed produces 200u in 2s");
    const auto rejected = [](const nlohmann::json& invalid) {
        try { phase1::Config::FromJson(invalid); }
        catch (const std::exception&) { return true; }
        return false;
    };
    for (const char* key : {"baseSpeed", "baseWidth", "baseHeight", "baseCenterX", "baseCenterY", "trialEndX"})
    {
        auto invalid = data; invalid.erase(key);
        require(rejected(invalid), "Missing base field");
    }
    auto invalid = data; invalid["baseSpeed"] = 0;
    require(rejected(invalid), "Zero speed");
    invalid = data; invalid["baseSpeed"] = 60; // P240 / B60 = 4、委任範囲外
    require(rejected(invalid), "Invalid speed ratio");
    invalid = data; invalid["trialEndX"] = 20000;
    require(rejected(invalid), "Endpoint outside world");
    invalid = data; invalid["baseCenterY"] = 0;
    require(rejected(invalid), "Base outside world");
    std::cout << "PASS: base motion, catch-up, endpoint, JSON values and rejection\n";
}
