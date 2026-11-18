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

        /// <summary>許可した生成間隔だけを重ね、比較条件を含む最終設定を検証する。</summary>
        /// <param name="base">基本のEnemySpawnConfig JSON。</param>
        /// <returns>EnemySpawnerに渡せる生成設定。</returns>
        object::EnemySpawnConfig BuildSpawnConfig(const nlohmann::json& base) const
        {
            auto merged = base;
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
                if (entry.key() != "interval") throw std::runtime_error("QuestConfig: unsupported spawn override");
            if (spawn.contains("interval"))
            {
                c.spawnInterval = spawn.at("interval").get<double>();
                if (!std::isfinite(*c.spawnInterval) || *c.spawnInterval < 3 || *c.spawnInterval > 6)
                    throw std::runtime_error("QuestConfig: interval must be 3..6 seconds");
            }
            return c;
        }
    };
}
