module;
#include <algorithm>
#include <cmath>

export module UI.PlayHud;
import UI.UIBase;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;
import Object.ObjectManager;
import <optional>;

export namespace ui
{
    /// <summary>地点マーカーと共有するHUD背景板の画面矩形を返す。</summary>
    /// <param name="showEnemy">最寄り敵の行を表示する場合true。</param>
    /// <returns>文字を保護する背景板の範囲（px）。</returns>
    inline shape::Rect PlayHudPanel(bool showEnemy)
    {
        return {{12, 12}, {578, showEnemy ? 240.0f : 212.0f}};
    }
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
        bool showNearestEnemy{};
        std::optional<math::Vector2f> toEnemy{};
    };

    /// <summary>拠点から最寄りの有効敵への差分を求める。同距離は安定ID順。</summary>
    /// <param name="objects">現在のPlayに登録されたオブジェクト。</param>
    /// <param name="baseCenter">拠点中心のワールド座標（u）。</param>
    /// <returns>拠点から敵中心への差分。敵がいなければ空。</returns>
    std::optional<math::Vector2f> NearestEnemyOffset(object::ObjectManager& objects, math::Vector2f baseCenter);

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

    /// <summary>UIManagerが所有するプレイHUD。ゲーム参照を持たず、そのフレームの値だけを保持する。</summary>
    class PlayHud final : public UIBase
    {
    public:
        /// <summary>描画直前に表示値を更新する。</summary>
        /// <param name="values">現在の実状態。</param>
        /// <param name="baseScreenRect">Cameraで変換済みの拠点矩形。</param>
        void SetValues(const HudValues& values, const shape::Rect& baseScreenRect)
        { m_values = values; m_baseScreenRect = baseScreenRect; }
        /// <summary>背景、数値、方向、操作案内を順に描く。</summary>
        void Draw() override;
    private:
        HudValues m_values{};
        shape::Rect m_baseScreenRect{};
    };

    /// <summary>確定したHPと進行率を表示するUI要素。</summary>
    class StageStatus final : public UIBase
    {
    public:
        /// <summary>確定値をコピーして保持する。</summary>
        /// <param name="health">終了時HP。</param>
        /// <param name="progress">終了時進行率（0〜1）。</param>
        StageStatus(float health, float progress) : m_health(health), m_progress(progress) {}
        /// <summary>プレイ中と共通の形式で確定値を描く。</summary>
        void Draw() override;
    private:
        float m_health, m_progress;
    };
}
