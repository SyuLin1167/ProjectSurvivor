export module Object.SpawnPosition;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;
import Object.EnemyConfig;
import <array>;
import <algorithm>;
import <cmath>;

export namespace spawnPosition
{
    inline constexpr float EdgeMargin = 48; // 描画範囲と画面端の最小余白（u）。
    inline constexpr float PlayerClearance = 256, EnemyClearance = 64, BaseClearance = 64;
    inline constexpr float ApproachSeconds = 15; // 接近を許容するゲーム秒。
    inline constexpr size_t CandidateCount = 24;

    /// <summary>可視矩形の四辺と護送ルート付近から有限個の候補を作る。</summary>
    /// <param name="view">ワールド座標の可視矩形。</param>
    /// <param name="baseCenter">護送対象の中心。カメラの追従対象とは独立する。</param>
    /// <param name="enemy">次に生成する個体の描画サイズ。</param>
    /// <param name="routeDistance">拠点付近の画面外候補までの距離（u）。</param>
    /// <returns>画面外・境界・安全距離の検証前の候補。</returns>
    inline std::array<math::Vector2f, CandidateCount> Candidates(const shape::Rect& view,
        math::Vector2f baseCenter, const combat::EnemyConfig& enemy, float routeDistance)
    {
        std::array<math::Vector2f, CandidateCount> points{};
        const auto bounds = enemy.VisualBounds({});
        // 描画矩形が非対称な旧ラベルでも、画面から全体が離れる位置へ中心を置く。
        for (int i = 0; i < 4; ++i)
        {
            const float t = (i + 1) / 5.0f;
            points[i * 4] = {view.Left() - bounds.Right() - EdgeMargin, view.Top() + view.size.y * t};
            points[i * 4 + 1] = {view.Right() - bounds.Left() + EdgeMargin, view.Top() + view.size.y * t};
            points[i * 4 + 2] = {view.Left() + view.size.x * t, view.Top() - bounds.Bottom() - EdgeMargin};
            points[i * 4 + 3] = {view.Left() + view.size.x * t, view.Bottom() - bounds.Top() + EdgeMargin};
        }
        // 探索で画面が拠点から離れた場合も、拠点前方のルートへ敵を供給する。
        const std::array<math::Vector2f, 8> offsets{{
            {1,0}, {1,-0.5f}, {1,0.5f}, {0.5f,-1}, {0.5f,1}, {0,-1}, {0,1}, {-1,0}}};
        for (int i = 0; i < 8; ++i) points[16+i] = baseCenter + offsets[i].Norm() * routeDistance;
        return points;
    }

    /// <summary>敵の全描画領域が余白付き可視矩形と重ならないか調べる。</summary>
    /// <param name="position">候補中心（u）。</param>
    /// <param name="enemy">描画形状。</param>
    /// <param name="view">現在の可視範囲。</param>
    /// <returns>完全に画面外ならtrue。</returns>
    inline bool Outside(math::Vector2f position, const combat::EnemyConfig& enemy, const shape::Rect& view)
    {
        const math::Vector2f margin{EdgeMargin - 0.01f, EdgeMargin - 0.01f};
        return !enemy.VisualBounds(position).AABB({view.pos - margin, view.size + margin * 2});
    }

    /// <summary>既存の直線追跡を粗い時間刻みで見積もり、遠すぎる候補を除外する。</summary>
    /// <param name="position">候補中心。</param>
    /// <param name="enemy">移動速度と円半径。</param>
    /// <param name="base">拠点の矩形。</param>
    /// <param name="baseSpeed">拠点の+X方向速度（u/s）。</param>
    /// <param name="endX">試行終了の拠点中心X。</param>
    /// <returns>15秒以内かつ終端前に接近する見込みがある場合true。停止対象には終端時間の制限を付けない。</returns>
    inline bool CanApproach(math::Vector2f position, const combat::EnemyConfig& enemy,
        shape::Rect base, float baseSpeed, float endX)
    {
        constexpr float step = 0.25f; // 候補選定時だけの見積もり。ゲームのdtは変更しない。
        float duration = ApproachSeconds;
        // 最終地点で停止した拠点にも同じ追跡見積もりを使い、ゼロ除算と残時間0での拒否を避ける。
        if (baseSpeed > 0)
            duration = (std::min)(ApproachSeconds, (endX - base.Center().x) / baseSpeed);
        for (float time = 0; time <= duration; time += step)
        {
            const math::Vector2f nearest{std::clamp(position.x, base.Left(), base.Right()),
                std::clamp(position.y, base.Top(), base.Bottom())};
            if (position.Distance(nearest) <= enemy.Radius) return true;
            const auto delta = base.Center() - position;
            position += delta.Norm() * (std::min)(delta.Length(), enemy.Speed * step);
            base.pos.x += baseSpeed * step;
        }
        return false;
    }
}
