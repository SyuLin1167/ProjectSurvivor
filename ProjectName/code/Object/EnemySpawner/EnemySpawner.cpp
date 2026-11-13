module Object.EnemySpawner;
import Object.Enemy;
import <array>;
import <vector>;
import <algorithm>;

namespace object
{
    EnemySpawner::EnemySpawner(EnemySpawnConfig config, phase2::EnemyConfig enemy, math::Vector2f worldSize)
        : m_config(config), m_enemy(enemy), m_worldSize(worldSize), m_remaining(config.firstDelay), m_random(config.seed)
    {
        // JSON以外から構築された場合も、承認範囲外の値を開始前に拒否する。
        m_config.Validate();
        // 能力の他項目は共有し、候補セットで明示されたHPだけを置き換える。
        if (m_config.healthOverride) m_enemy.Health = *m_config.healthOverride;
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
            constexpr std::array kinds{EnemyKind::Normal, EnemyKind::Runner, EnemyKind::Normal, EnemyKind::Heavy};
            slot.emplace(center, m_enemy, m_config.alphaCombat ? kinds[m_nextKind] : EnemyKind::Classic);
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
        m_elapsed += dt;
        m_remaining -= dt;
        if (m_remaining > 0.000001) return false;
        // 予約済み期限の区間を使うため、20/40秒を跨いでも期限を繰り上げない。
        const double due = m_elapsed + m_remaining;
        const double interval = m_config.IntervalAt(due);
        const auto directionsOrder = NextDirections();
        const int requested = m_config.CountAt(due);
        int generated = 0;
        // 通常の端数は保持。複数期限を跨ぐ遅延は回収せず現在区間から予約し直す。
        m_remaining = m_remaining > -interval ? interval + m_remaining : m_config.IntervalAt(m_elapsed);
        // 生存かつ有効な敵だけを収集し、同時上限と候補位置の安全距離に使用する。
        std::vector<math::Vector2f> centers;
        for (const auto& object : manager.GetObjForTag(ObjectTag::ENEMY))
            if (const auto enemy = std::dynamic_pointer_cast<Enemy>(object); enemy && enemy->CanBeAttacked())
                centers.push_back(enemy->Center());
        // 同時上限未満の場合だけ候補を調べる。上限に達していれば末尾の見送り処理へ進む。
        if (centers.size() < static_cast<size_t>(m_config.limit))
        {
            // 正面・右上・右下を巡回する。斜めも同じ生成距離にするため方向を正規化する。
            const std::array<math::Vector2f, 3> directions{
                math::Vector2f{1, 0}, math::Vector2f{1, -1}.Norm(), math::Vector2f{1, 1}.Norm()};
            const auto baseCenter = base.Center();
            const auto half = base.Size().Half();
            // 各方向は1回だけ調べる。必要数・空き枠に達したら終了し、追加抽選はしない。
            for (int candidate = 0; candidate < 3 && generated < requested &&
                centers.size() < static_cast<size_t>(m_config.limit); ++candidate)
            {
                const auto position = baseCenter + directions[directionsOrder[candidate]] * m_config.distance;
                const float radius = m_enemy.Radius;
                // 円がマップからはみ出す、またはプレイヤーに近すぎる場合は次の候補へ進む。
                if (position.x < radius || position.y < radius || position.x > m_worldSize.x - radius ||
                    position.y > m_worldSize.y - radius || position.Distance(playerCenter) < 256) continue;
                // 生成時の安全判定のみ。移動・押し戻し・接触通知は既存Colliderへ任せる。
                // 拠点の矩形内で候補中心に最も近い点を求め、敵円との重なりを調べる。
                const math::Vector2f nearest{std::clamp(position.x, baseCenter.x - half.x, baseCenter.x + half.x),
                    std::clamp(position.y, baseCenter.y - half.y, baseCenter.y + half.y)};
                // 拠点に接触する位置、または生存敵との中心間距離が64u未満の位置は採用しない。
                if (position.Distance(nearest) <= radius || std::any_of(centers.begin(), centers.end(),
                    [position](const auto& center) { return position.Distance(center) < 64; })) continue;
                // 安全な位置が確定してからプールを取得する。空きがなければ今回の生成を見送る。
                auto enemy = Acquire(position);
                if (!enemy) break; // 撃破個体への所有参照が残る場合も、使用中領域を上書きしない。
                // 参照解決とCollider登録後に、同機会の後続候補の距離判定・枠数へ含める。
                manager.AddInitializedObject(std::move(enemy));
                if (m_config.alphaCombat) m_nextKind = (m_nextKind + 1) % 4;
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
