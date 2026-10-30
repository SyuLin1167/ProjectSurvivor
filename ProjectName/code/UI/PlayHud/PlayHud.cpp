module;
#include <DxLib.h>
#include <cmath>
#include <algorithm>
#include <format>

module UI.PlayHud;
import GameSystem.CameraGeometry;
import GameSystem.Window;
import UI.DisplayElements;

namespace ui
{
    /// <summary>位置とワールド差分から同じ形の方向矢印を描く。</summary>
    /// <param name="position">矢印中心の画面座標（px）。</param>
    /// <param name="delta">Pから拠点への差分（u）。</param>
    void DrawBaseArrow(math::Vector2f position, math::Vector2f delta)
    {
        if (delta.x == 0 && delta.y == 0) return;
        const auto direction = delta.Norm();
        const math::Vector2f side{-direction.y, direction.x};
        const auto tip = position + direction * 14.0f;
        const auto left = position - direction * 10.0f + side * 8.0f;
        const auto right = position - direction * 10.0f - side * 8.0f;
        DrawTriangle(static_cast<int>(tip.x), static_cast<int>(tip.y),
            static_cast<int>(left.x), static_cast<int>(left.y),
            static_cast<int>(right.x), static_cast<int>(right.y), GetColor(255, 190, 80), true);
    }

    /// <summary>文字要素を共通描画へ渡す。内容の整形はゲーム側に閉じ込める。</summary>
    /// <param name="text">整形済み文字列。</param>
    /// <param name="x">左端（px）。</param>
    /// <param name="y">上端（px）。</param>
    /// <param name="color">描画色。</param>
    void DrawLabel(const std::string& text, float x, float y, unsigned int color = 0xffffff)
    {
        TextLabel label(text, color);
        label.SetPosition({x, y});
        label.Draw();
    }

    void DrawStageStatus(float health, float progress, int x, int y)
    {
        DrawLabel(std::format("CARAVAN HP: {:.0f} / 100", health), static_cast<float>(x), static_cast<float>(y));
        Gauge gauge;
        gauge.SetPosition({static_cast<float>(x), static_cast<float>(y + 24)});
        gauge.SetSize({240, 12});
        gauge.SetValue(health, 100);
        gauge.Draw();
        DrawLabel(std::format("PROGRESS: {}%", ProgressPercent(progress)), static_cast<float>(x), static_cast<float>(y + 48));
    }

    void StageStatus::Draw()
    {
        if (IsVisible()) DrawStageStatus(m_health, m_progress, static_cast<int>(m_pos.x), static_cast<int>(m_pos.y));
    }

    void PlayHud::Draw()
    {
        if (!IsVisible()) return;
        const auto& values = m_values;
        const auto& baseScreenRect = m_baseScreenRect;
        const auto screen = gameSystem::Window::GetWindowData()->SIZE;
        const shape::Rect viewport{{0, 0}, {static_cast<float>(screen.x), static_cast<float>(screen.y)}};
        const auto marker = cameraGeometry::BaseMarker(baseScreenRect, viewport);
        // Object描画後に背景板ごと重ね、カメラや敵の描画順で文字が隠れないようにする。
        DrawBox(12, 12, 590, 200, GetColor(15, 20, 25), true);
        DrawStageStatus(values.health, values.progress, 24, 24);
        // 距離の表示丸めでも到達を早めない。残距離は到達まで最低1uと表示する。
        DrawLabel(std::format("TRAVEL: {}u / {}u | REMAIN: {}u",
            static_cast<int>(std::floor(values.distance)), static_cast<int>(std::round(values.distance + values.remaining)),
            static_cast<int>(std::ceil(values.remaining))), 24, 100);
        DrawBaseArrow({38, 140}, values.toBase);
        DrawLabel(std::format("BASE: {} | {:.1f}u | {} | {}",
            DirectionText(values.toBase), values.baseDistance, values.isNear ? "NEAR" : "FAR",
            marker.offscreen ? "OFFSCREEN" : "ONSCREEN"), 64, 132, GetColor(255, 190, 80));
        DrawLabel("WASD / arrows: move | Auto attack | R: Title", 24, 174);
        if (marker.offscreen)
        {
            // 画面外判定は画面全体。配置だけHUDより下の領域へ収め、上端で重ならないようにする。
            const shape::Rect markerArea{{0, 210}, {static_cast<float>(screen.x), static_cast<float>(screen.y - 210)}};
            const auto position = cameraGeometry::BaseMarker(baseScreenRect, markerArea).position;
            DrawBaseArrow(position, values.toBase);
            DrawLabel("BASE", position.x - 20, position.y + 18, GetColor(255, 190, 80));
        }
    }
}
