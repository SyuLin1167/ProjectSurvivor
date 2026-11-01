module Object.EnemySpawner;
import Object.Enemy;
import <array>;
import <vector>;
import <algorithm>;

namespace object
{
    EnemySpawner::EnemySpawner(EnemySpawnConfig config, phase2::EnemyConfig enemy, math::Vector2f worldSize)
        : m_config(config), m_enemy(enemy), m_worldSize(worldSize), m_remaining(config.firstDelay)
    {
        // JSON以外から構築された場合も、承認範囲外の値を開始前に拒否する。
        m_config.Validate();
    }

    std::shared_ptr<Enemy> EnemySpawner::Acquire(math::Vector2f center)
    {
        // 使用中の領域は飛ばし、先頭から最初の空き領域を探す。
        for (size_t i = 0; i < m_pool->slots.size(); ++i)
        {
            auto& slot = m_pool->slots[i];
            if (slot) continue;
            // 再利用する領域へ新個体を構築し、HP・接触時計・対象参照を初期状態に戻す。
            slot.emplace(center, m_enemy);
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
        m_remaining -= dt;
        if (m_remaining > 0.000001) return false;
        // この機会の開始方向を保存し、生成の成否にかかわらず次回の方向を1つ進める。
        const int start = m_direction;
        m_direction = (m_direction + 1) % 3;
        // 通常の端数のみ保持し、大きな遅延にも連続生成ループを使わない。
        m_remaining = m_config.interval + (m_remaining > -m_config.interval ? m_remaining : 0.0);
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
            // 開始方向から最大3候補を試し、安全な位置が見つかった時点で探索を終える。
            for (int candidate = 0; candidate < 3; ++candidate)
            {
                const auto position = baseCenter + directions[(start + candidate) % 3] * m_config.distance;
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
                // 参照解決とCollider登録を完了して管理へ渡す。1体登録できた場合はtrueを返す。
                manager.AddInitializedObject(std::move(enemy));
                return true; // 登録時は行動させず、次フレームの通常更新で扱う。
            }
        }
        // 上限／全候補不成立／プール空きなしは1周期待つ。空きができても即座に埋め戻さない。
        m_remaining = m_config.interval;
        return false;
    }
}
