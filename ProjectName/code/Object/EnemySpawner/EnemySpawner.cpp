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
        m_config.Validate();
    }

    bool EnemySpawner::Update(float dt, ObjectManager& manager, const MovingBase& base, math::Vector2f playerCenter)
    {
        if (!std::isfinite(dt) || dt <= 0 || base.Health() <= 0 || (base.NowState() & MainState::DEAD)) return false;
        m_remaining -= dt;
        if (m_remaining > 0.000001) return false;
        const int start = m_direction;
        m_direction = (m_direction + 1) % 3;
        // 通常の端数のみ保持し、大きな遅延にも連続生成ループを使わない。
        m_remaining = m_config.interval + (m_remaining > -m_config.interval ? m_remaining : 0.0);
        std::vector<math::Vector2f> centers;
        for (const auto& object : manager.GetObjForTag(ObjectTag::ENEMY))
            if (const auto enemy = std::dynamic_pointer_cast<Enemy>(object); enemy && enemy->CanBeAttacked())
                centers.push_back(enemy->Center());
        if (centers.size() < static_cast<size_t>(m_config.limit))
        {
            const std::array<math::Vector2f, 3> directions{
                math::Vector2f{1, 0}, math::Vector2f{1, -1}.Norm(), math::Vector2f{1, 1}.Norm()};
            const auto baseCenter = base.Center();
            const auto half = base.Size().Half();
            for (int candidate = 0; candidate < 3; ++candidate)
            {
                const auto position = baseCenter + directions[(start + candidate) % 3] * m_config.distance;
                const float radius = m_enemy.Radius;
                if (position.x < radius || position.y < radius || position.x > m_worldSize.x - radius ||
                    position.y > m_worldSize.y - radius || position.Distance(playerCenter) < 256) continue;
                // 生成時の安全判定のみ。移動・押し戻し・接触通知は既存Colliderへ任せる。
                const math::Vector2f nearest{std::clamp(position.x, baseCenter.x - half.x, baseCenter.x + half.x),
                    std::clamp(position.y, baseCenter.y - half.y, baseCenter.y + half.y)};
                if (position.Distance(nearest) <= radius || std::any_of(centers.begin(), centers.end(),
                    [position](const auto& center) { return position.Distance(center) < 64; })) continue;
                manager.AddInitializedObject(std::make_unique<Enemy>(position, m_enemy));
                return true; // 登録時は行動させず、次フレームの通常更新で扱う。
            }
        }
        // 上限／全候補不成立は1周期待つ。空きができても即座に埋め戻さない。
        m_remaining = m_config.interval;
        return false;
    }
}
