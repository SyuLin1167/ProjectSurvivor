module;
#include <json.hpp>
#include <cmath>
#include <stdexcept>
export module Quest.QuestConfig;
import MyLib.File.JsonIO;
import <string>;
import <optional>;
import Object.EnemySpawner;

export namespace quest
{
    /// <summary>依頼JSONの検証済み値。目的地と依頼のIDは独立して保持する。</summary>
    struct QuestConfig
    {
        std::string id, destinationId, name, description; // 表示文字列はUTF-8。
        float initialHealth, maxHealth; // 今回の試行のHP。
        std::optional<double> spawnInterval; // 秒。省略時は基本生成設定を継承。

        std::optional<std::string> spawnMode; // classicなら基準比較、offscreenなら段階生成。
        std::optional<int> spawnLimit; // offscreenの10/20体条件。

        /// <summary>許可したモード・上限・基本間隔を重ね、最終設定を検証する。</summary>
        /// <param name="base">基本のEnemySpawnConfig JSON。</param>
        /// <returns>EnemySpawnerに渡せる生成設定。</returns>
        object::EnemySpawnConfig BuildSpawnConfig(const nlohmann::json& base) const
        {
            auto merged = base;
            if (spawnMode)
            {
                merged["mode"] = *spawnMode;
                if (*spawnMode == "offscreen")
                {
                    // 旧C1の段階数・周期は新密度と混在させず、基本JSON自体は復帰用に残す。
                    merged["balanceSet"] = "baseline";
                    merged["limit"] = spawnLimit.value_or(20);
                }
            }
            if (spawnInterval) merged["interval"] = *spawnInterval;
            return object::EnemySpawnConfig::FromJson(merged);
        }

        /// <summary>必須型・範囲と対応目的地を検証する。</summary>
        /// <param name="data">依頼JSON。</param>
        /// <returns>適用可能な依頼値。不正設定は例外で通知する。</returns>
        static QuestConfig FromJson(const nlohmann::json& data)
        {
            QuestConfig c{data.at("id").get<std::string>(), data.at("destinationId").get<std::string>(),
                data.at("name").get<std::string>(), data.at("description").get<std::string>(),
                data.at("initialHealth").get<float>(), data.at("maxHealth").get<float>(), {}};
            if (c.id.empty() || c.name.empty() || c.description.empty() || c.destinationId != "road")
                throw std::runtime_error("QuestConfig: invalid identity or destination");
            if (!std::isfinite(c.initialHealth) || !std::isfinite(c.maxHealth) ||
                c.initialHealth <= 0 || c.initialHealth > c.maxHealth || c.maxHealth > 1000)
                throw std::runtime_error("QuestConfig: invalid health");
            const auto& spawn = data.at("enemySpawn");
            if (!spawn.is_object()) throw std::runtime_error("QuestConfig: enemySpawn must be an object");
            // 未対応の上書きを黙って無視せず、入力境界で誤記を通知する。
            for (const auto& entry : spawn.items())
                if (entry.key() != "interval" && entry.key() != "mode" && entry.key() != "limit") throw std::runtime_error("QuestConfig: unsupported spawn override");
            if (spawn.contains("interval"))
            {
                c.spawnInterval = spawn.at("interval").get<double>();
                if (!std::isfinite(*c.spawnInterval) || *c.spawnInterval < 3 || *c.spawnInterval > 6)
                    throw std::runtime_error("QuestConfig: interval must be 3..6 seconds");
            }
            if (spawn.contains("mode"))
            {
                c.spawnMode = spawn.at("mode").get<std::string>();
                if (*c.spawnMode != "classic" && *c.spawnMode != "offscreen")
                    throw std::runtime_error("QuestConfig: unknown spawn mode");
            }
            if (spawn.contains("limit"))
            {
                if (!spawn.at("limit").is_number_integer() || c.spawnMode != "offscreen")
                    throw std::runtime_error("QuestConfig: limit requires offscreen mode");
                c.spawnLimit = spawn.at("limit").get<int>();
                if (*c.spawnLimit != 10 && *c.spawnLimit != 20)
                    throw std::runtime_error("QuestConfig: offscreen limit must be 10 or 20");
            }
            return c;
        }
    };
}
