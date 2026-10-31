export module Object.EnemySpawner;
import MyLib.File.JsonIO;
import MyLib.Math.Vector2;
import Object.EnemyConfig;
import Object.ObjectManager;
import Object.MovingBase;
import <cmath>;
import <stdexcept>;

export namespace object
{
    /// <summary>DEC-03補足で調整委任された出現値。安全距離・方向・生成数は固定。</summary>
    struct EnemySpawnConfig
    {
        double interval = 4, firstDelay = 3;
        float distance = 480;
        int limit = 8;
        /// <summary>有限値・整数上限と承認範囲を確認する。</summary>
        void Validate() const
        {
            if (!std::isfinite(interval) || interval < 3 || interval > 6 ||
                !std::isfinite(firstDelay) || firstDelay < 2 || firstDelay > 5 ||
                !std::isfinite(distance) || distance < 384 || distance > 640 || limit < 6 || limit > 12)
                throw std::runtime_error("EnemySpawnConfig: outside DEC-03 approval");
        }
        /// <summary>新規Play開始時にJSONを検証して読み込む。</summary>
        /// <param name="data">出現設定オブジェクト。</param>
        /// <returns>承認範囲内の出現設定。</returns>
        static EnemySpawnConfig FromJson(const json& data)
        {
            if (!data.at("limit").is_number_integer()) throw std::runtime_error("EnemySpawnConfig: limit must be integer");
            EnemySpawnConfig c{data.at("interval").get<double>(), data.at("firstDelay").get<double>(),
                data.at("distance").get<float>(), data.at("limit").get<int>()};
            c.Validate();
            return c;
        }
    };

    /// <summary>新規Playごとの出現時計と方向。更新・終了判定の後にのみ呼び出す。</summary>
    class EnemySpawner final
    {
    public:
        /// <summary>設定を保持し、初回待機時間と正面方向から開始する。</summary>
        /// <param name="config">承認範囲の出現設定。</param>
        /// <param name="enemy">既存の敵能力設定。</param>
        /// <param name="worldSize">マップの幅・高さ（u）。</param>
        EnemySpawner(EnemySpawnConfig config, phase2::EnemyConfig enemy, math::Vector2f worldSize);
        /// <summary>継続中のフレーム末尾で最大1体を生成する。見送り分は蓄積しない。</summary>
        /// <param name="dt">共通ゲーム経過秒数。停止中は0。</param>
        /// <param name="manager">更新・Stepが完了したシーンの管理。</param>
        /// <param name="base">存続中の拠点。終了判定は呼び出し側で先に行う。</param>
        /// <param name="playerCenter">P中心のワールド座標（u）。</param>
        /// <returns>新しい敵1体の登録に成功した場合true。</returns>
        bool Update(float dt, ObjectManager& manager, const MovingBase& base, math::Vector2f playerCenter);
    private:
        EnemySpawnConfig m_config;
        phase2::EnemyConfig m_enemy;
        math::Vector2f m_worldSize;
        double m_remaining;
        int m_direction = 0;
    };
}
