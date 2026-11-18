export module Quest.QuestCatalog;
export import Quest.QuestConfig;
import AppContext;
import MyLib.File.FileSystem;
import <array>;
import <string_view>;
import <stdexcept>;

export namespace quest
{
    /// <summary>選択可能なIDと相対JSON名を対応付ける一覧登録。</summary>
    struct CatalogEntry { std::string_view id; const char* file; };
    inline constexpr std::array AvailableQuests{
        CatalogEntry{"pilgrim", "Quests/Pilgrim.json"},
        CatalogEntry{"cargo", "Quests/Cargo.json"}
    };

    /// <summary>登録IDを解決し、最新のJSONを検証して読み込む。</summary>
    /// <param name="id">選択画面またはRetryから渡されたID。</param>
    /// <returns>この読込時点の依頼のコピー。</returns>
    inline QuestConfig FindQuest(std::string_view id)
    {
        for (const auto& entry : AvailableQuests)
        {
            if (entry.id != id) continue;
            auto& files = gameSystem::AppCtx::FileSystem();
            auto config = QuestConfig::FromJson(files.jsonIO.Load(files.GetDataDir() / entry.file));
            if (config.id != id) throw std::runtime_error("QuestCatalog: ID differs from registered file");
            return config;
        }
        throw std::invalid_argument("QuestCatalog: unknown quest ID");
    }
}
