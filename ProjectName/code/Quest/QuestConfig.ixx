module;
#include <json.hpp>
#include <cmath>
#include <stdexcept>
export module Quest.QuestConfig;
import MyLib.File.JsonValue;
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
        std::optional<int> spawnLimit; // offscreenの生存上限（配布12体）。
        std::optional<std::string> requiredClear; // 解放に必要な全区間クリアID。省略時は初期解放。

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
                    merged["limit"] = spawnLimit.value_or(12);
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
            QuestConfig c{file::jsonValue::Read<std::string>(data, "id"), file::jsonValue::Read<std::string>(data, "destinationId"),
                file::jsonValue::Read<std::string>(data, "name"), file::jsonValue::Read<std::string>(data, "description"),
                file::jsonValue::Read<float>(data, "initialHealth"), file::jsonValue::Read<float>(data, "maxHealth"), {}};
            if (c.id.empty() || c.name.empty() || c.description.empty() || c.destinationId != "road")
                throw std::runtime_error("QuestConfig: invalid identity or destination");
            if (!std::isfinite(c.initialHealth) || !std::isfinite(c.maxHealth) ||
                c.initialHealth <= 0 || c.initialHealth > c.maxHealth || c.maxHealth > 1000)
                throw std::runtime_error("QuestConfig: invalid health");
            const auto& spawn = file::jsonValue::Member(data, "enemySpawn");
            if (!spawn.is_object()) throw std::runtime_error("QuestConfig: enemySpawn must be an object");
            // 未対応の上書きを黙って無視せず、入力境界で誤記を通知する。
            for (const auto& entry : spawn.items())
                if (entry.key() != "interval" && entry.key() != "mode" && entry.key() != "limit") throw std::runtime_error("QuestConfig: unsupported spawn override");
            c.spawnInterval = file::jsonValue::ReadOptional<double>(spawn, "interval");
            if (c.spawnInterval)
            {
                if (!std::isfinite(*c.spawnInterval) || *c.spawnInterval < 3 || *c.spawnInterval > 6)
                    throw std::runtime_error("QuestConfig: interval must be 3..6 seconds");
            }
            c.spawnMode = file::jsonValue::ReadOptional<std::string>(spawn, "mode");
            if (c.spawnMode)
            {
                if (*c.spawnMode != "classic" && *c.spawnMode != "offscreen")
                    throw std::runtime_error("QuestConfig: unknown spawn mode");
            }
            if (spawn.contains("limit"))
            {
                if (!file::jsonValue::Member(spawn, "limit").is_number_integer() || c.spawnMode != "offscreen")
                    throw std::runtime_error("QuestConfig: limit requires offscreen mode");
                c.spawnLimit = file::jsonValue::Read<int>(spawn, "limit");
                if (*c.spawnLimit != 10 && *c.spawnLimit != 12 && *c.spawnLimit != 20)
                    throw std::runtime_error("QuestConfig: offscreen limit must be 10, 12 or 20");
            }
            c.requiredClear = file::jsonValue::ReadOptional<std::string>(data, "requiredClear");
            if (c.requiredClear && (c.requiredClear->empty() || *c.requiredClear == c.id))
                throw std::runtime_error("QuestConfig: invalid unlock requirement");
            return c;
        }
    };
}
