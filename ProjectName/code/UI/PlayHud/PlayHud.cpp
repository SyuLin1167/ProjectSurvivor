module;
#include <DxLib.h>
#include <cmath>
#include <algorithm>
#include <format>

module UI.PlayHud;
import GameSystem.CameraGeometry;
import GameSystem.Window;
import UI.DisplayElements;
import Object.Enemy;

namespace ui
{
    std::optional<math::Vector2f> NearestEnemyOffset(object::ObjectManager& objects, math::Vector2f baseCenter)
    {
        std::optional<math::Vector2f> nearest;
        float bestSquared{};
        uint64_t bestId{};
        // 表示範囲やプレイヤーの射程に依存せず、撃破済み・無効な個体を除く。
        for (const auto& item : objects.GetObjForTag(object::ENEMY))
        {
            const auto enemy = std::dynamic_pointer_cast<object::Enemy>(item);
            if (!enemy || !enemy->CanBeAttacked()) continue;
            const auto delta = enemy->Center() - baseCenter;
            const float squared = delta.x * delta.x + delta.y * delta.y;
            if (!nearest || squared < bestSquared || (squared == bestSquared && enemy->TargetId() < bestId))
            {
                nearest = delta;
                bestSquared = squared;
                bestId = enemy->TargetId();
            }
        }
        return nearest;
    }

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
        // 探索案内はy=170。操作案内と最寄り敵を独立した行に配置する。
        const auto panel = PlayHudPanel(values.showNearestEnemy);
        const int panelBottom = static_cast<int>(panel.Bottom());
        DrawBox(static_cast<int>(panel.Left()), static_cast<int>(panel.Top()),
            static_cast<int>(panel.Right()), panelBottom, GetColor(15, 20, 25), true);
        DrawStageStatus(values.health, values.progress, 24, 24);
        // 距離の表示丸めでも到達を早めない。残距離は到達まで最低1uと表示する。
        DrawLabel(std::format("TRAVEL: {}u / {}u | REMAIN: {}u",
            static_cast<int>(std::floor(values.distance)), static_cast<int>(std::round(values.distance + values.remaining)),
            static_cast<int>(std::ceil(values.remaining))), 24, 100);
        DrawBaseArrow({38, 140}, values.toBase);
        DrawLabel(std::format("BASE: {} | {:.1f}u | {} | {}",
            DirectionText(values.toBase), values.baseDistance, values.isNear ? "NEAR" : "FAR",
            marker.offscreen ? "OFFSCREEN" : "ONSCREEN"), 64, 132, GetColor(255, 190, 80));
        DrawLabel("WASD / arrows: move | Auto attack | R: Title", 24, 198);
        if (values.showNearestEnemy)
        {
            const auto& delta = values.toEnemy;
            DrawLabel(delta ? std::format("最寄り敵：{} | {:.1f}u（拠点基準）", DirectionText(*delta), delta->Length())
                            : "最寄り敵：なし", 24, 226);
        }
        if (marker.offscreen)
        {
            // 画面外判定は画面全体。配置だけHUDより下の領域へ収め、上端で重ならないようにする。
            const float markerTop = static_cast<float>(panelBottom + 10);
            const shape::Rect markerArea{{0, markerTop}, {static_cast<float>(screen.x), screen.y - markerTop}};
            const auto position = cameraGeometry::BaseMarker(baseScreenRect, markerArea).position;
            DrawBaseArrow(position, values.toBase);
            DrawLabel("BASE", position.x - 20, position.y + 18, GetColor(255, 190, 80));
        }
    }
}
