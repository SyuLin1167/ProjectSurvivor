export module Object.EnemyConfig;

import MyLib.File.JsonIO;
import MyLib.Math.Vector2;
import <array>;
import <vector>;
import <cmath>;
import <stdexcept>;

export namespace phase2
{
    /// <summary>敵能力の試作設定。JSONの値を検証してから生成・移動・描画へ渡す。</summary>
    struct EnemyConfig
    {
        float Health{};
        float Radius{}; // u
        float Speed{};  // u/s
        enum class Look { Classic, Normal, Runner, Heavy };
        Look look{Look::Classic};

        /// <summary>DEC-02と#19比較試遊の許可値・速度範囲を確認する。不正時は例外を送出する。</summary>
        /// <param name="data">EnemyConfig.jsonのオブジェクト。欠損・型不正も拒否する。</param>
        /// <returns>検証済みの敵設定。攻撃・接触ダメージの設定は含まない。</returns>
        static EnemyConfig FromJson(const json& data)
        {
            EnemyConfig c;
            c.Health = data.at("health").get<float>();
            c.Radius = data.at("radius").get<float>();
            c.Speed = data.at("speed").get<float>();
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
    /// <summary>敵の能力と外見を開始時に登録し、生成に成功した順で次の種類を選ぶ。</summary>
    class EnemyCatalog
    {
        using Config = phase2::EnemyConfig;
        std::array<Config, 3> types{};
        unsigned cursor{};
        bool mixed{};
    public:
        /// <summary>通常敵を登録し、Alphaで使用する追加種だけをJSONから展開する。</summary>
        /// <param name="base">比較用HPを反映した通常敵。</param>
        /// <param name="variants">Alpha用の俊足・重装設定。通常モードでは空。</param>
        EnemyCatalog(Config base, const json* variants)
        {
            types[0] = base;
            if (!variants) return;
            mixed = true;
            types[0].look = Config::Look::Normal;
            auto read = [&](const char* key, Config::Look look)
            {
                Config type = base;
                type.look = look;
                type.Health = variants->at(key).at("health").get<float>();
                type.Speed = variants->at(key).at("speed").get<float>();
                if (!std::isfinite(type.Health) || type.Health <= 0 ||
                    !std::isfinite(type.Speed) || type.Speed <= 0)
                    throw std::runtime_error("Invalid alpha enemy parameters");
                return type;
            };
            types[1] = read("runner", Config::Look::Runner);
            types[2] = read("heavy", Config::Look::Heavy);
        }
        /// <summary>安全位置の検査と生成で同じ敵設定を参照する。</summary>
        /// <returns>次に生成する敵の能力・外見。</returns>
        const Config& Next() const
        {
            constexpr std::array order{0, 1, 0, 2};
            return types[mixed ? order[cursor] : 0];
        }
        /// <summary>登録成功時だけ通常・俊足・通常・重装の次枠へ進む。</summary>
        void Registered() { cursor = (cursor + 1) % 4; }
    };
}
