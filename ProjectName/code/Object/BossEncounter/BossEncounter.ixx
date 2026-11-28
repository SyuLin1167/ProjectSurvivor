export module Object.BossEncounter;
import Object.Enemy;
import Object.EnemyConfig;
import Object.ObjectManager;
import Object.MovingBase;
import Object.SpawnPosition;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;
import <memory>;
import <optional>;
import <limits>;
import <algorithm>;

export namespace object
{
    /// <summary>最終区間のボス1体と出現結果を管理する。通常敵プールやシーンを所有しない。</summary>
    class BossEncounter
    {
    public:
        /// <summary>区間専用の出現前状態を作る。敵オブジェクトの生成は終端到達後に行う。</summary>
        /// <param name="config">読込境界で検証済みの共通ボス設定。</param>
        /// <param name="worldSize">検証済みマップの幅・高さ（u）。</param>
        BossEncounter(combat::EnemyConfig config, math::Vector2f worldSize)
            : m_config(config), m_worldSize(worldSize) {}

        /// <summary>停止した拠点の近くに、安全な画面外候補を探して1体だけ登録する。再呼出しは何もしない。</summary>
        /// <param name="objects">更新・Collider処理を完了した、このPlayの所有者。</param>
        /// <param name="base">終端で停止済みの有効な拠点。</param>
        /// <param name="playerCenter">プレイヤーのワールド中心。</param>
        /// <param name="view">更新後のカメラ可視矩形。</param>
        void Start(ObjectManager& objects, const MovingBase& base, math::Vector2f playerCenter, const shape::Rect& view)
        {
            if (Started()) return;
            const auto center = base.Center();
            const shape::Rect baseRect{center - base.Size().Half(), base.Size()};
            const auto enemies = objects.GetObjForTag(ObjectTag::ENEMY);
            std::optional<math::Vector2f> best;
            float bestDistance = (std::numeric_limits<float>::max)();
            // 通常生成と同じ周辺候補を使い、停止中の拠点へ最短で接近できる安全候補を選ぶ。
            for (const auto position : spawnPosition::Candidates(view, center, m_config, 480))
            {
                const auto bounds = m_config.VisualBounds(position);
                if (!spawnPosition::Outside(position, m_config, view) ||
                    bounds.Left() < 0 || bounds.Top() < 0 ||
                    bounds.Right() > m_worldSize.x || bounds.Bottom() > m_worldSize.y) continue;
                if (position.Distance(playerCenter) < spawnPosition::PlayerClearance) continue;
                const math::Vector2f nearest{std::clamp(position.x, baseRect.Left(), baseRect.Right()),
                    std::clamp(position.y, baseRect.Top(), baseRect.Bottom())};
                if (position.Distance(nearest) <= m_config.Radius + spawnPosition::BaseClearance) continue;
                // ボスは通常敵より大きいので、固定中心間距離だけでなく双方の円半径を使う。
                const bool overlapsEnemy = std::any_of(enemies.begin(), enemies.end(), [&](const auto& object) {
                    const auto enemy = std::dynamic_pointer_cast<Enemy>(object);
                    return enemy && enemy->CanBeAttacked() && position.Distance(enemy->Center()) <
                        (std::max)(spawnPosition::EnemyClearance, m_config.Radius + enemy->Radius() + 16);
                });
                if (overlapsEnemy || !spawnPosition::CanApproach(position, m_config, baseRect, 0, base.EndX())) continue;
                const float distance = position.Distance(center);
                if (distance < bestDistance) { best = position; bestDistance = distance; }
            }
            if (!best)
            {
                // 毎フレームの無期限再試行をせず、Playが理由表示とTitle復帰を担当する。
                m_phase = Phase::SpawnFailed;
                return;
            }
            // 通常プールに入れない1体を既存ObjectManagerへ登録。撃破後もHP0の事実を所有参照で保持する。
            auto enemy = std::make_shared<Enemy>(*best, m_config);
            objects.AddInitializedObject(enemy);
            m_enemy = std::move(enemy);
            m_phase = Phase::Fighting;
        }

        /// <summary>出現処理を一度実施済みか返す。</summary>
        /// <returns>戦闘中または出現失敗ならtrue。</returns>
        bool Started() const { return m_phase != Phase::Approach; }
        /// <summary>安全な出現位置がなく、通常進行を停止すべきか返す。</summary>
        /// <returns>明示的な復帰案内が必要ならtrue。</returns>
        bool Failed() const { return m_phase == Phase::SpawnFailed; }
        /// <summary>生成済みのボスの撃破を確認する。未生成を成功と扱わない。</summary>
        /// <returns>戦闘開始済みでHP0ならtrue。</returns>
        bool Defeated() const { return m_phase == Phase::Fighting && m_enemy->Health() <= 0; }
        /// <summary>戦闘中のHP表示に使用する実値を返す。</summary>
        /// <returns>現在HP。Start成功後にだけ呼び出す。</returns>
        float Health() const { return m_enemy->Health(); }
        /// <summary>HPバーの上限を返す。</summary>
        /// <returns>読込済みの初期HP。</returns>
        float MaxHealth() const { return m_config.Health; }

    private:
        enum class Phase { Approach, Fighting, SpawnFailed };
        Phase m_phase = Phase::Approach;
        combat::EnemyConfig m_config;
        math::Vector2f m_worldSize;
        // PlayのメンバーなのでSceneBaseのObjectManagerより先に解放され、Collider所有者を越えて残らない。
        std::shared_ptr<Enemy> m_enemy;
    };
}