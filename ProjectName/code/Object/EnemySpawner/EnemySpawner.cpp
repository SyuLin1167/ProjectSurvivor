module Object.EnemySpawner;
import Object.Enemy;
import <array>;
import <vector>;
import <algorithm>;
import GameSystem.FrameRate;
import GameSystem.Camera;
import Object.SpawnPosition;
import MyLib.Shape.Rect;

namespace object
{
    namespace
    {
        /// <summary>上限判定と距離判定で共有する、生存敵の中心を集める。</summary>
        /// <param name="manager">現在の試行の所有者。</param>
        /// <param name="capacity">事前確保する最大個体数。</param>
        /// <returns>撃破済み・無効な個体を除いた中心座標。</returns>
        std::vector<math::Vector2f> CollectActiveCenters(ObjectManager& manager, int capacity)
        {
            std::vector<math::Vector2f> centers;
            centers.reserve(capacity);
            for (const auto& object : manager.GetObjForTag(ObjectTag::ENEMY))
            {
                const auto enemy = std::dynamic_pointer_cast<Enemy>(object);
                if (enemy && enemy->CanBeAttacked()) centers.push_back(enemy->Center());
            }
            return centers;
        }

        /// <summary>候補の成立条件を調べる。失敗はエラーではなく今回の候補の見送り。</summary>
        /// <param name="position">生成中心。</param>
        /// <param name="enemy">次の個体の描画範囲・半径・速度。</param>
        /// <param name="offscreen">画面外と接近見積もりを必要とするモード。</param>
        /// <param name="view">現在の可視矩形。</param>
        /// <param name="worldSize">マップ寸法。</param>
        /// <param name="base">追跡対象。現在の矩形と終端を使う。</param>
        /// <param name="playerCenter">プレイヤー中心。</param>
        /// <param name="centers">この生成機会で登録済みの個体も含む生存敵の中心。</param>
        /// <returns>すべての生成条件を満たす場合true。状態変更は行わない。</returns>
        bool IsSafePosition(math::Vector2f position, const combat::EnemyConfig& enemy,
            bool offscreen, const shape::Rect& view, math::Vector2f worldSize,
            const MovingBase& base, math::Vector2f playerCenter,
            const std::vector<math::Vector2f>& centers)
        {
            if (offscreen)
            {
                // AABBは画面との重なりに使用。マップ内の完全包含は別条件として残す。
                if (!spawnPosition::Outside(position, enemy, view)) return false;
                const auto bounds = enemy.VisualBounds(position);
                if (bounds.Left() < 0 || bounds.Top() < 0 ||
                    bounds.Right() > worldSize.x || bounds.Bottom() > worldSize.y) return false;
            }
            // 描画サイズとColliderのサイズは別の設定なので、円の包含も保証する。
            const float radius = enemy.Radius;
            if (position.x < radius || position.y < radius ||
                position.x > worldSize.x - radius || position.y > worldSize.y - radius) return false;
            if (position.Distance(playerCenter) < spawnPosition::PlayerClearance) return false;

            const auto baseCenter = base.Center();
            const auto half = base.Size().Half();
            const math::Vector2f nearest{std::clamp(position.x, baseCenter.x - half.x, baseCenter.x + half.x),
                std::clamp(position.y, baseCenter.y - half.y, baseCenter.y + half.y)};
            float clearance = radius;
            if (offscreen) clearance += spawnPosition::BaseClearance;
            if (position.Distance(nearest) <= clearance) return false;
            if (std::any_of(centers.begin(), centers.end(), [position](const auto& center) {
                return position.Distance(center) < spawnPosition::EnemyClearance;
            })) return false;

            // 反復計算は安価な除外判定の後に行う。候補順や採用条件は変えない。
            if (offscreen)
                return spawnPosition::CanApproach(position, enemy,
                    {baseCenter - half, base.Size()}, base.Speed(), base.EndX());
            return true;
        }
    }

    EnemySpawner::EnemySpawner(EnemySpawnConfig config, combat::EnemyConfig enemy, math::Vector2f worldSize, const json* variants, const std::vector<size_t>& order)
        : m_config(config), m_catalog({config.healthOverride.value_or(enemy.Health), enemy.Radius, enemy.Speed}, variants, order), m_worldSize(worldSize), m_remaining(config.firstDelay), m_random(config.seed)
    {
        // JSON以外から構築された場合も、承認範囲外の値を開始前に拒否する。
        m_config.Validate();
        m_pool = std::make_shared<Pool>(m_config.PoolCapacity());
    }

    std::array<int, 3> EnemySpawner::NextDirections()
    {
        // 基準では既存の固定巡回を完全に維持する。
        if (!m_config.randomDirections)
        {
            const int start = m_direction;
            m_direction = (m_direction + 1) % 3;
            return {start, (start + 1) % 3, (start + 2) % 3};
        }
        // mt19937と6順列の割当を固定し、標準ライブラリのshuffle実装差を避ける。
        static constexpr std::array<std::array<int, 3>, 6> orders{{
            {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}}};
        // 2^32を6で割った余り4値だけを再抽選し、剰余による偏りを除く。
        // 抽選は安全候補の確認前に完結し、上限見送りでも同じように進む。
        uint32_t value;
        do { value = static_cast<uint32_t>(m_random()); } while (value >= 4294967292u);
        return orders[value % 6];
    }

    std::shared_ptr<Enemy> EnemySpawner::Acquire(math::Vector2f center)
    {
        // 使用中の領域は飛ばし、先頭から最初の空き領域を探す。
        for (size_t i = 0; i < m_pool->slots.size(); ++i)
        {
            auto& slot = m_pool->slots[i];
            if (slot) continue;
            // 再利用する領域へ新個体を構築し、HP・接触時計・対象参照を初期状態に戻す。
            slot.emplace(center, m_catalog.Next());
            // 領域は再利用するが個体の寿命は分ける。古いweak_ptrを新個体へ復活させない。
            // deleterがプールを保持するため、Spawnerが先に破棄されても領域は有効。
            return std::shared_ptr<Enemy>(&*slot, [pool = m_pool, i](Enemy*) { pool->slots[i].reset(); });
        }
        // 全領域に所有参照が残っている場合は、空の参照を返して生成を見送らせる。
        return {};
    }

    bool EnemySpawner::Update(float dt, ObjectManager& manager, const MovingBase& base, math::Vector2f playerCenter)
    {
        // 停止中・不正な経過時間・終了済みの拠点では、生成時計を進めずfalseを返す。
        if (!std::isfinite(dt) || dt <= 0 || base.Health() <= 0 || (base.NowState() & MainState::DEAD)) return false;
        // 次の生成期限まで待つ。微小な誤差で生成が1フレーム遅れないよう許容幅を設ける。
        gameSystem::FrameRate::CountUp(m_elapsed, dt);
        if (!gameSystem::FrameRate::Countdown(m_remaining, dt, true)) return false;
        // 予約済み期限の区間を使うため、段階境界を跨いでも期限を繰り上げない。
        const double due = m_elapsed + m_remaining;
        const double interval = m_config.IntervalAt(due);
        std::array<int, 3> directionsOrder{};
        int candidateCount;
        int candidateStart = 0;
        if (m_config.offscreen)
        {
            // 24候補を7つずつずらし、同じ辺からの出現への偏りを抑える。
            candidateCount = int(spawnPosition::CandidateCount);
            candidateStart = m_direction;
            m_direction = (m_direction + 7) % candidateCount;
        }
        else
        {
            // C1の乱数消費順は、上限で見送る機会も含め従来どおり維持する。
            candidateCount = 3;
            directionsOrder = NextDirections();
        }
        const int limit = m_config.LimitAt(due);
        const auto view = gameSystem::Camera::Instance().VisibleRect();
        const int requested = m_config.CountAt(due);
        int generated = 0;
        // 通常の端数は保持。複数期限を跨ぐ遅延は回収せず現在区間から予約し直す。
        if (m_remaining > -interval)
            m_remaining += interval;
        else
            m_remaining = m_config.IntervalAt(m_elapsed);
        auto centers = CollectActiveCenters(manager, m_config.limit);
        // 同時上限未満の場合だけ候補を調べる。上限に達していれば末尾の見送り処理へ進む。
        if (centers.size() < static_cast<size_t>(limit))
        {
            // 旧比較は正面・右上・右下を巡回する。斜めも同じ生成距離にするため方向を正規化する。
            const std::array<math::Vector2f, 3> directions{
                math::Vector2f{1, 0}, math::Vector2f{1, -1}.Norm(), math::Vector2f{1, 1}.Norm()};
            const auto baseCenter = base.Center();
            // 各方向は1回だけ調べる。必要数・空き枠に達したら終了し、追加抽選はしない。
            for (int candidate = 0; candidate < candidateCount && generated < requested &&
                centers.size() < static_cast<size_t>(limit); ++candidate)
            {
                const auto& next = m_catalog.Next();
                math::Vector2f position;
                if (m_config.offscreen)
                {
                    const auto candidates = spawnPosition::Candidates(view, baseCenter, next, m_config.distance);
                    position = candidates[(candidateStart + candidate) % candidateCount];
                }
                else
                {
                    position = baseCenter + directions[directionsOrder[candidate]] * m_config.distance;
                }
                if (!IsSafePosition(position, next, m_config.offscreen, view, m_worldSize,
                    base, playerCenter, centers)) continue;
                // 安全な位置が確定してからプールを取得する。空きがなければ今回の生成を見送る。
                auto enemy = Acquire(position);
                if (!enemy) break; // 撃破個体への所有参照が残る場合も、使用中領域を上書きしない。
                // 参照解決とCollider登録後に、同機会の後続候補の距離判定・枠数へ含める。
                manager.AddInitializedObject(std::move(enemy));
                m_catalog.Registered();
                centers.push_back(position);
                ++generated; // どの個体も登録時は行動させず、次フレームの通常更新で扱う。
            }
        }
        // 部分生成も成功。既存の予約期限を維持し、不足分の再試行や繰り越しはしない。
        if (generated > 0) return true;
        // 上限／全候補不成立／プール空きなしは1周期待つ。空きができても即座に埋め戻さない。
        m_remaining = m_config.IntervalAt(m_elapsed);
        return false;
    }
}
