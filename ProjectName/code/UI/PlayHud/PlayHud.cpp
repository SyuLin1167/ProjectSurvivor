module;
#include <DxLib.h>
#include <cmath>
#include <algorithm>

module UI.PlayHud;
import GameSystem.CameraGeometry;
import GameSystem.Window;

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

    void DrawStageStatus(float health, float progress, int x, int y)
    {
        const auto color = GetColor(255, 255, 255);
        DrawFormatString(x, y, color, "CARAVAN HP: %.0f / 100", health);
        DrawBox(x, y + 24, x + 240, y + 36, GetColor(70, 70, 70), true);
        const int fill = static_cast<int>(240 * std::clamp(health / 100, 0.0f, 1.0f));
        if (fill > 0) DrawBox(x, y + 24, x + fill, y + 36, GetColor(255, 190, 80), true);
        DrawBox(x, y + 24, x + 240, y + 36, color, false);
        DrawFormatString(x, y + 48, color, "PROGRESS: %d%%", ProgressPercent(progress));
    }

    void DrawPlayHud(const HudValues& values, const shape::Rect& baseScreenRect)
    {
        const auto screen = gameSystem::Window::GetWindowData()->SIZE;
        const shape::Rect viewport{{0, 0}, {static_cast<float>(screen.x), static_cast<float>(screen.y)}};
        const auto marker = cameraGeometry::BaseMarker(baseScreenRect, viewport);
        // Object描画後に背景板ごと重ね、カメラや敵の描画順で文字が隠れないようにする。
        DrawBox(12, 12, 590, 200, GetColor(15, 20, 25), true);
        DrawStageStatus(values.health, values.progress, 24, 24);
        // 距離の表示丸めでも到達を早めない。残距離は到達まで最低1uと表示する。
        DrawFormatString(24, 100, GetColor(255, 255, 255), "TRAVEL: %du / %du | REMAIN: %du",
            static_cast<int>(std::floor(values.distance)), static_cast<int>(std::round(values.distance + values.remaining)),
            static_cast<int>(std::ceil(values.remaining)));
        DrawBaseArrow({38, 140}, values.toBase);
        DrawFormatString(64, 132, GetColor(255, 190, 80), "BASE: %s | %.1fu | %s | %s",
            DirectionText(values.toBase), values.baseDistance, values.isNear ? "NEAR" : "FAR",
            marker.offscreen ? "OFFSCREEN" : "ONSCREEN");
        DrawString(24, 174, "WASD / arrows: move | Auto attack | R: Title", GetColor(255, 255, 255));
        if (marker.offscreen)
        {
            // 画面外判定は画面全体。配置だけHUDより下の領域へ収め、上端で重ならないようにする。
            const shape::Rect markerArea{{0, 210}, {static_cast<float>(screen.x), static_cast<float>(screen.y - 210)}};
            const auto position = cameraGeometry::BaseMarker(baseScreenRect, markerArea).position;
            DrawBaseArrow(position, values.toBase);
            DrawString(static_cast<int>(position.x - 20), static_cast<int>(position.y + 18), "BASE", GetColor(255, 190, 80));
        }
    }
}
