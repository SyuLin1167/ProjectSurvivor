module;
#include <DxLib.h>
#include <algorithm>

module Object.Map;

import Scene.StageConfig;
import GameSystem.Camera;
import Asset.AlphaSprites;
import GameSystem.Window;

using namespace gameSystem;
using namespace math;

namespace object
{
    Map::Map(const stage::StageConfig& config)
        : m_stageConfig(config) {}
    void Map::Init()
    {
        // 試作ではJSONのワールド寸法をそのまま使用し、グリッド数への丸めを避ける。
        Camera::Instance().SetWorldSize({m_stageConfig.WorldWidth, m_stageConfig.WorldHeight});
    }

    void Map::Draw()
    {
        // 画面内のグリッドだけ描画し、大きなワールドでも描画本数を抑える。
        auto& camera = Camera::Instance();
        const auto offset = camera.GetOffset();
        const auto screen = Window::GetWindowData()->SIZE;
        const int spacing = m_stageConfig.GridSize;
        const int startX = (std::max)(0, static_cast<int>(offset.x) / spacing * spacing);
        const int startY = (std::max)(0, static_cast<int>(offset.y) / spacing * spacing);
        const int endX = static_cast<int>((std::min)(m_stageConfig.WorldWidth, offset.x + screen.x));
        const int endY = static_cast<int>((std::min)(m_stageConfig.WorldHeight, offset.y + screen.y));
        if (m_stageConfig.alphaCombat)
        {
            // 共通タイルを可視範囲だけ敷き、拠点の街道は轍で示す。
            for (int y = startY; y < endY; y += spacing)
                for (int x = startX; x < endX; x += spacing)
                {
                    const auto p = camera.WorldToScreen(float(x), float(y));
                    alphaSprites::Draw(alphaSprites::Sprite::Ground, int(p.x), int(p.y), spacing);
                }
            const auto road = camera.WorldToScreen(0, m_stageConfig.BaseCenter.y);
            for (int track : {-22, 22})
                DrawLine(0, int(road.y) + track, int(screen.x), int(road.y) + track, GetColor(92, 81, 57), 3);
            return;
        }
        for (int x = startX; x <= endX; x += spacing)
        {
            const auto p = camera.WorldToScreen(static_cast<float>(x), 0);
            DrawLine(static_cast<int>(p.x), 0, static_cast<int>(p.x),
                static_cast<int>(screen.y), GetColor(45, 55, 65));
        }
        for (int y = startY; y <= endY; y += spacing)
        {
            const auto p = camera.WorldToScreen(0, static_cast<float>(y));
            DrawLine(0, static_cast<int>(p.y), static_cast<int>(screen.x),
                static_cast<int>(p.y), GetColor(45, 55, 65));
        }
        const auto origin = camera.WorldToScreen(0, 0);
        const auto end = camera.WorldToScreen(m_stageConfig.WorldWidth - 1, m_stageConfig.WorldHeight - 1);
        DrawBox(static_cast<int>(origin.x), static_cast<int>(origin.y),
            static_cast<int>(end.x), static_cast<int>(end.y), GetColor(220, 220, 220), false);
    }
}
