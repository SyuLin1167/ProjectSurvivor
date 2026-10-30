module;
#include <algorithm>
#include <cmath>

export module UI.PlayHud;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;

export namespace ui
{
    /// <summary>そのフレームの実状態から作るHUD値。ゲームオブジェクトを所有しない。</summary>
    struct HudValues
    {
        float health;
        float progress;
        float distance;
        float remaining;
        math::Vector2f toBase;
        float baseDistance;
        bool isNear;
    };

    /// <summary>状態値を表示範囲へ制限し、既存の中心間距離・NEAR基準を求める。</summary>
    /// <param name="health">拠点の現在HP。</param>
    /// <param name="progress">StageStateの進行率（0〜1）。</param>
    /// <param name="distance">StageStateの進行距離（u）。</param>
    /// <param name="remaining">StageStateの残距離（u）。</param>
    /// <param name="toBase">プレイヤー中心から拠点中心への差分（u）。</param>
    /// <returns>描画用の値。NEARは勝敗や接触判定には使わない。</returns>
    inline HudValues MakeHudValues(float health, float progress, float distance, float remaining,
        math::Vector2f toBase)
    {
        const float baseDistance = toBase.Length();
        return {std::clamp(health, 0.0f, 100.0f), std::clamp(progress, 0.0f, 1.0f),
            (std::max)(0.0f, distance), (std::max)(0.0f, remaining), toBase, baseDistance, baseDistance <= 128};
    }

    /// <summary>到達前に100%へ丸め上げない共通表示値を返す。</summary>
    /// <param name="progress">内部進行率（0〜1）。</param>
    /// <returns>0〜100の整数パーセント。</returns>
    inline int ProgressPercent(float progress)
    {
        return static_cast<int>(std::floor(std::clamp(progress, 0.0f, 1.0f) * 100));
    }

    /// <summary>色や矢印だけに依存しない8方向の短い文字表示を返す。</summary>
    /// <param name="delta">Pから拠点へのワールド差分。Y正方向は下。</param>
    /// <returns>方向文字列。同一点はHERE。</returns>
    inline const char* DirectionText(math::Vector2f delta)
    {
        if (delta.x == 0 && delta.y == 0) return "HERE";
        if (std::abs(delta.y) <= std::abs(delta.x) * 0.414214f) return delta.x > 0 ? "RIGHT" : "LEFT";
        if (std::abs(delta.x) <= std::abs(delta.y) * 0.414214f) return delta.y > 0 ? "DOWN" : "UP";
        if (delta.y > 0) return delta.x > 0 ? "DOWN-RIGHT" : "DOWN-LEFT";
        return delta.x > 0 ? "UP-RIGHT" : "UP-LEFT";
    }

    /// <summary>HP数値・バーと進行率をPlayとResultで同じ形式で描く。</summary>
    /// <param name="health">拠点HP（0〜100）。</param>
    /// <param name="progress">進行率（0〜1）。</param>
    /// <param name="x">画面上の左端（px）。</param>
    /// <param name="y">画面上の上端（px）。</param>
    void DrawStageStatus(float health, float progress, int x, int y);

    /// <summary>状態・距離・操作を画面座標で重ね、既存マーカーを再利用する。</summary>
    /// <param name="values">同フレームの実状態から作った表示値。</param>
    /// <param name="baseScreenRect">Cameraで変換済みの拠点矩形。</param>
    void DrawPlayHud(const HudValues& values, const shape::Rect& baseScreenRect);
}
