export module Object.EnemyConfig;
import MyLib.File.JsonValue;

import MyLib.File.JsonIO;
import MyLib.Math.Vector2;
import <array>;
import <string>;
import <unordered_map>;
import MyLib.Shape.Rect;
import <algorithm>;
import <vector>;
import <cmath>;
import <stdexcept>;

export namespace combat
{
    /// <summary>敵能力の試作設定。JSONの値を検証してから生成・移動・描画へ渡す。</summary>
    struct EnemyConfig
    {
        float Health{};
        float Radius{}; // u
        float Speed{};  // u/s
        enum class Look { Classic, Normal, Runner, Heavy, Charger, Boss };
        Look look{Look::Classic};
        float chargeMultiplier = 1; // 通常速度に対する加速倍率。1なら従来の追跡のみ。
        double chargeWait = 3, chargeDuration = 0.6; // 通常移動と加速を交互に繰り返すゲーム秒。

        float contactDamage = 10; // 通常敵は従来値。ボスのみJSONで変更する。
        double contactInterval = 1; // 敵ごとの接触攻撃間隔（ゲーム秒）。

        /// <summary>通常追跡と加速の境界を跨ぐフレームも、時間の割合どおりの移動量を返す。</summary>
        /// <param name="elapsed">この個体が追跡した累積ゲーム秒。</param>
        /// <param name="dt">今回の非負ゲーム経過秒。</param>
        /// <returns>追跡方向へ進む距離（u）。Colliderによる押し戻し前の値。</returns>
        float TravelDistance(double elapsed, float dt) const
        {
            if (chargeMultiplier == 1) return Speed * dt;
            // 周期内の加速時間を積分し、30/60/120fpsで境界を丸めることによる距離差を避ける。
            const double cycle = chargeWait + chargeDuration;
            const auto acceleratedTime = [&](double time) {
                return std::floor(time / cycle) * chargeDuration +
                    (std::max)(0.0, std::fmod(time, cycle) - chargeWait);
            };
            const double burst = acceleratedTime(elapsed + dt) - acceleratedTime(elapsed);
            return float(Speed * (dt + (chargeMultiplier - 1) * burst));
        }

        /// <summary>現在の加速表示を移動と同じ周期で判定する。</summary>
        /// <param name="elapsed">個体の追跡経過秒。</param>
        /// <returns>短時間加速中ならtrue。</returns>
        bool IsCharging(double elapsed) const
        {
            return chargeMultiplier > 1 && std::fmod(elapsed, chargeWait + chargeDuration) >= chargeWait;
        }

        /// <summary>敵本体と旧モードの名前表示を囲む描画範囲を求める。</summary>
        /// <param name="center">敵中心（u）。</param>
        /// <returns>画面外判定に使用するワールド矩形。</returns>
        shape::Rect VisualBounds(math::Vector2f center) const
        {
            if (look == Look::Boss) return {center - math::Vector2f{Radius, Radius}, {Radius * 2, Radius * 2}};
            if (look != Look::Classic) return {center - math::Vector2f{16, 16}, {32, 32}};
            // 旧ENEMYラベルは右と上へ張り出すため、本体円より広く確保する。
            return {center - math::Vector2f{Radius, Radius * 2},
                {(std::max)(Radius * 2, 128.0f), (std::max)(Radius * 3, 64.0f)}};
        }

        /// <summary>共通ボスの能力を入力境界で検証する。通常敵の固定値制約とは分離する。</summary>
        /// <param name="data">BossConfig.jsonの必須項目。</param>
        /// <returns>追跡・Collider・接触攻撃で共用する検証済み設定。</returns>
        static EnemyConfig BossFromJson(const json& data)
        {
            namespace jv = file::jsonValue;
            const auto positive = [](float value) { return std::isfinite(value) && value > 0; };
            EnemyConfig config;
            config.look = Look::Boss;
            config.Health = jv::Read<float>(data, "health", positive, "must be positive and finite");
            config.Radius = jv::Read<float>(data, "radius",
                [](float value) { return std::isfinite(value) && value >= 32 && value <= 96; }, "must be 32..96 units");
            config.Speed = jv::Read<float>(data, "speed", positive, "must be positive and finite");
            config.contactDamage = jv::Read<float>(data, "contactDamage", positive, "must be positive and finite");
            config.contactInterval = jv::Read<double>(data, "contactInterval",
                [](double value) { return std::isfinite(value) && value >= 0.1; }, "must be finite and at least 0.1 seconds");
            return config;
        }

        /// <summary>DEC-02と#19比較試遊の許可値・速度範囲を確認する。不正時は例外を送出する。</summary>
        /// <param name="data">EnemyConfig.jsonのオブジェクト。欠損・型不正も拒否する。</param>
        /// <returns>検証済みの敵設定。攻撃・接触ダメージの設定は含まない。</returns>
        static EnemyConfig FromJson(const json& data)
        {
            EnemyConfig c;
            c.Health = file::jsonValue::Read<float>(data, "health");
            c.Radius = file::jsonValue::Read<float>(data, "radius");
            c.Speed = file::jsonValue::Read<float>(data, "speed");
            // #19の比較限定でHP20を許可する。基準HP30を維持し、中間値や段階変化は許可しない。
            if ((c.Health != 30 && c.Health != 20) || c.Radius != 16 || !std::isfinite(c.Speed) ||
                c.Speed < 100 || c.Speed > 140)
                throw std::runtime_error("EnemyConfig: values outside DEC-02 / issue 19 approval");
            return c;
        }
    };
}

export namespace object
{
    /// <summary>成功した生成だけで巡回を進める敵カタログ。区間開始時に設定を確定する。</summary>
    class EnemyCatalog
    {
        using Config = combat::EnemyConfig;
        std::array<Config, 4> m_types{};
        std::vector<size_t> m_order{0}; // 同じ種類の重複数が1巡の配分になる。
        size_t m_cursor{};
    public:
        /// <summary>外部の種類名を固定カタログの番号へ解決する。</summary>
        /// <param name="name">JSONのnormal/runner/heavy/charger。</param>
        /// <returns>能力表の番号。不明な種類は入力境界でout_of_range。</returns>
        static size_t TypeIndex(const std::string& name)
        {
            static const std::unordered_map<std::string, size_t> types{
                {"normal", 0}, {"runner", 1}, {"heavy", 2}, {"charger", 3}};
            return types.at(name);
        }

        /// <summary>Alphaの能力表と検証済み区間巡回を保持する。比較用モードは従来どおり。</summary>
        /// <param name="base">比較用HPを反映した通常敵。</param>
        /// <param name="variants">Alpha用の追加種JSON。非Alphaでは空。</param>
        /// <param name="order">種類番号の巡回。空なら従来の通常・俊足・通常・重装。</param>
        EnemyCatalog(Config base, const json* variants, const std::vector<size_t>& order = {})
        {
            m_types[0] = base;
            if (!variants) return;
            m_types[0].look = Config::Look::Normal;
            m_order = order;
            if (m_order.empty()) m_order = {0, 1, 0, 2};
            const auto read = [&](const char* key, Config::Look look) {
                const auto& data = file::jsonValue::Member(*variants, key);
                const auto positive = [](float value) { return std::isfinite(value) && value > 0; };
                Config type = base;
                type.look = look;
                type.Health = file::jsonValue::Read<float>(data, "health", positive, "must be positive and finite");
                type.Speed = file::jsonValue::Read<float>(data, "speed", positive, "must be positive and finite");
                return type;
            };
            m_types[1] = read("runner", Config::Look::Runner);
            m_types[2] = read("heavy", Config::Look::Heavy);
            // 旧Alpha JSONは追加種を参照しない限り有効。突進型を使う区間だけ必須値を読み込む。
            if (std::find(m_order.begin(), m_order.end(), 3) != m_order.end())
            {
                auto& charger = m_types[3];
                charger = read("charger", Config::Look::Charger);
                const auto& data = file::jsonValue::Member(*variants, "charger");
                charger.chargeMultiplier = file::jsonValue::Read<float>(data, "chargeMultiplier",
                    [](float value) { return std::isfinite(value) && value > 1 && value <= 3; }, "must be in (1, 3]");
                const auto seconds = [](double value) { return std::isfinite(value) && value >= 0.1 && value <= 60; };
                charger.chargeWait = file::jsonValue::Read<double>(data, "chargeWait", seconds, "must be 0.1..60 seconds");
                charger.chargeDuration = file::jsonValue::Read<double>(data, "chargeDuration", seconds, "must be 0.1..60 seconds");
            }
        }
        /// <summary>安全位置の検査と実生成で同じ敵設定を参照する。</summary>
        /// <returns>次に生成する敵の能力・外見。</returns>
        const Config& Next() const { return m_types.at(m_order[m_cursor]); }
        /// <summary>登録成功時だけ次の種類へ進む。見送りは配分を消費しない。</summary>
        void Registered() { m_cursor = (m_cursor + 1) % m_order.size(); }
    };
}