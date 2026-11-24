export module Object.EnemySpawner;
import MyLib.File.JsonIO;
import MyLib.Math.Vector2;
import Object.EnemyConfig;
import Object.Enemy;
import <array>;
import <vector>;
import <algorithm>;
import <optional>;
import <memory>;
import Object.ObjectManager;
import Object.MovingBase;
import <cmath>;
import <stdexcept>;
import <random>;
import <cstdint>;
import <string>;

export namespace object
{
    /// <summary>旧比較と画面外生成の出現値。開始時にモードごとの範囲を検証する。</summary>
    struct EnemySpawnConfig
    {
        // ゲーム時間の秒数。初回待機と通常の出現周期を区別する。
        double interval = 4, firstDelay = 3;
        float distance = 480;
        int limit = 8;
        // 比較セット選択時だけ有効。未選択時は従来のEnemyConfigと固定周期・巡回を使う。
        std::optional<float> healthOverride;
        bool stagedIntervals = false, randomDirections = false;
        uint32_t seed = 1901;
        bool stagedCount = false; // #19 v2のみ、期限20秒以降を最大2体にする。
        bool offscreen = false; // 画面外の段階生成。旧比較はfalse。
        /// <summary>段階ごとの生存上限を返す。</summary>
        /// <param name="time">予約期限のゲーム秒。</param>
        /// <returns>画面外モードは6→8→設定上限、旧比較は固定上限。</returns>
        int LimitAt(double time) const
        {
            if (!offscreen) return limit;
            if (time < 20) return (std::min)(limit, 6);
            if (time < 45) return (std::min)(limit, 8);
            return limit;
        }
        /// <summary>寿命のある個体の保持枠数を返す。</summary>
        /// <returns>旧比較は12、画面外モードは設定上限（配布12、将来用20にも対応）。</returns>
        size_t PoolCapacity() const
        {
            if (offscreen) return size_t(limit);
            return 12;
        }
        /// <summary>有限値・整数上限と承認範囲を確認する。</summary>
        void Validate() const
        {
            // 読込時だけ検証する。補正して比較条件を変えず、原因の項目を通知する。
            if (!std::isfinite(interval) || interval < 3 || interval > 6)
                throw std::runtime_error("EnemySpawnConfig: interval must be 3..6 seconds");
            if (!std::isfinite(firstDelay) || firstDelay < 2 || firstDelay > 5)
                throw std::runtime_error("EnemySpawnConfig: firstDelay must be 2..5 seconds");
            if (!std::isfinite(distance) || distance < 384 || distance > 640)
                throw std::runtime_error("EnemySpawnConfig: distance must be 384..640 units");
            if (offscreen)
            {
                if (limit != 10 && limit != 12 && limit != 20) throw std::runtime_error("Offscreen mode requires limit 10, 12 or 20");
                if (stagedIntervals || stagedCount || healthOverride)
                    throw std::runtime_error("Offscreen mode cannot mix comparison presets");
                return;
            }
            if (limit < 6 || limit > 12)
                throw std::runtime_error("EnemySpawnConfig: classic limit must be 6..12");
            // #19の個別復帰はHP30/20のみ。候補の共通値を他の調整と混在させない。
            if (healthOverride && ((*healthOverride != 20 && *healthOverride != 30) ||
                interval != 4 || firstDelay != 3 || distance != 480 || limit != 8))
                throw std::runtime_error("EnemySpawnConfig: outside issue 19 comparison");
            // 生成数だけを比較するv2は、段階周期やHP・方向条件の変更と混在させない。
            if (stagedCount && (stagedIntervals || healthOverride != 20 || !randomDirections ||
                seed != 1901 || interval != 4 || firstDelay != 3 || distance != 480 || limit != 8))
                throw std::runtime_error("EnemySpawnConfig: staged count requires approved C1 conditions");
        }
        /// <summary>処理フレームではなく予約期限から、この機会の生成数上限を決める。</summary>
        /// <param name="due">新規Play開始からの予約期限（ゲーム秒）。</param>
        /// <returns>基準・C0は1、C1の20秒以降は2。画面外は45秒から1→2。</returns>
        int CountAt(double due) const
        {
            if (offscreen)
            {
                if (due < 45) return 1;
                return 2;
            }
            if (stagedCount && due >= 20) return 2;
            return 1;
        }
        /// <summary>予約期限が属する区間の間隔を返す。境界通過だけでは予約を変更しない。</summary>
        /// <param name="time">新規Play開始からのゲーム秒数。</param>
        /// <returns>次回予約の秒数。画面外は20/45秒で2.5→2→1.5秒。旧候補は20/40秒で3/2.5秒。</returns>
        double IntervalAt(double time) const
        {
            if (offscreen)
            {
                if (time < 20) return 2.5;
                if (time < 45) return 2.0;
                return 1.5;
            }
            if (!stagedIntervals) return interval;
            if (time >= 40) return 2.5;
            if (time >= 20) return 3.0;
            return 4.0;
        }
        /// <summary>新規Play開始時にJSONを検証して読み込む。</summary>
        /// <param name="data">出現設定オブジェクト。</param>
        /// <returns>承認範囲内の出現設定。</returns>
        static EnemySpawnConfig FromJson(const json& data)
        {
            // 小数の上限が整数変換で切り捨てられないよう、変換前に型を確認する。
            if (!data.at("limit").is_number_integer()) throw std::runtime_error("EnemySpawnConfig: limit must be integer");
            // 必須項目を型付きで読み込む。欠落や型不一致はJSONの例外で通知する。
            EnemySpawnConfig c{data.at("interval").get<double>(), data.at("firstDelay").get<double>(),
                data.at("distance").get<float>(), data.at("limit").get<int>()};
            const auto mode = data.value("mode", std::string("classic"));
            if (mode != "classic" && mode != "offscreen") throw std::runtime_error("EnemySpawnConfig: unknown mode");
            c.offscreen = mode == "offscreen";
            // 旧JSONは基準として読める。候補の選択は起動前の明示指定に限定する。
            const auto selected = data.value("balanceSet", std::string("baseline"));
            if (selected != "baseline" && selected != "candidate")
                throw std::runtime_error("EnemySpawnConfig: unknown balanceSet");
            if (selected == "candidate")
            {
                const auto& candidate = data.at("candidate");
                c.healthOverride = candidate.at("health").get<float>();
                c.stagedIntervals = candidate.at("stagedIntervals").get<bool>();
                c.randomDirections = candidate.at("randomDirections").get<bool>();
                c.stagedCount = candidate.value("stagedCount", false);
                // 負数・小数・32bitを超える種は、切り捨てず開始前に拒否する。
                const auto& seedValue = candidate.at("seed");
                if (!seedValue.is_number_integer() || seedValue.get<double>() < 0 ||
                    seedValue.get<double>() > 4294967295.0)
                    throw std::runtime_error("EnemySpawnConfig: seed must be uint32");
                c.seed = seedValue.get<uint32_t>();
            }
            // 全項目が承認範囲内であることを確認してから、利用側へ返す。
            c.Validate();
            return c;
        }
    };

    /// <summary>新規Playごとの出現時計と方向。更新・終了判定の後にのみ呼び出す。</summary>
    class EnemySpawner final
    {
    public:
        /// <summary>設定を保持し、初回待機時間・巡回方向・専用乱数を初期化する。</summary>
        /// <param name="config">承認範囲の出現設定。</param>
        /// <param name="enemy">既存の敵能力設定。</param>
        /// <param name="worldSize">マップの幅・高さ（u）。</param>
        /// <param name="variants">Alphaの敵種定義。構築中だけ使用する。</param>
        EnemySpawner(EnemySpawnConfig config, combat::EnemyConfig enemy, math::Vector2f worldSize, const json* variants = nullptr);
        /// <summary>継続中のフレーム末尾で1機会を処理し、設定に応じ最大2体を生成する。見送り分は蓄積しない。</summary>
        /// <param name="dt">共通ゲーム経過秒数。停止中は0。</param>
        /// <param name="manager">更新・Stepが完了したシーンの管理。</param>
        /// <param name="base">存続中の拠点。終了判定は呼び出し側で先に行う。</param>
        /// <param name="playerCenter">P中心のワールド座標（u）。</param>
        /// <returns>新しい敵を1体以上登録した場合true。部分生成も成功として扱う。</returns>
        bool Update(float dt, ObjectManager& manager, const MovingBase& base, math::Vector2f playerCenter);
        /// <summary>撃破済みでも所有参照が残る枠を含め、占有中の数を返す。</summary>
        /// <returns>生存数とは独立したプールの占有数。</returns>
        size_t OccupiedSlots() const
        {
            return std::count_if(m_pool->slots.begin(), m_pool->slots.end(), [](const auto& slot) { return slot.has_value(); });
        }
    private:
        /// <summary>開始時に確保する格納領域。使用中スロットは最後の所有参照が解放されるまで再利用しない。</summary>
        struct Pool
        {
            // 空のoptionalを未使用領域として扱い、resetで個体を破棄して返却する。
            /// <summary>開始時に全枠を確保し、個体が生存する間は再配置しない。</summary>
            /// <param name="capacity">検証済みの最大占有枠数。</param>
            explicit Pool(size_t capacity) : slots(capacity) {}
            std::vector<std::optional<Enemy>> slots;
        };
        /// <summary>空き領域に新個体を構築し、最後の所有参照の解放時に破棄して領域を返す。</summary>
        /// <param name="center">生成中心のワールド座標（u）。</param>
        /// <returns>新個体の所有参照。空きがなければ空。</returns>
        std::shared_ptr<Enemy> Acquire(math::Vector2f center);
        /// <summary>生成機会ごとの候補順を進める。見送りや安全判定に乱数消費を依存させない。</summary>
        /// <returns>正面0・右上1・右下2を重複なく並べた候補順。</returns>
        std::array<int, 3> NextDirections();
        std::shared_ptr<Pool> m_pool;
        EnemySpawnConfig m_config;
        EnemyCatalog m_catalog;
        math::Vector2f m_worldSize;
        double m_remaining;
        double m_elapsed = 0;
        std::mt19937 m_random;
        int m_direction = 0;
    };
}
