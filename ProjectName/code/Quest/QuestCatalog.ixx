export module Quest.QuestCatalog;
export import Quest.QuestConfig;
import AppContext;
import MyLib.File.FileSystem;
import <array>;
import <unordered_map>;
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
        // 表示順の一覧を正本とし、ID検索用の索引は初回呼び出し時に一度だけ作る。
        // string_viewの参照先は一覧内の文字列リテラルなので、索引より長く存続する。
        static const auto filesById = [] {
            std::unordered_map<std::string_view, const char*> index;
            index.reserve(AvailableQuests.size());
            for (const auto& entry : AvailableQuests)
            {
                if (!index.emplace(entry.id, entry.file).second)
                    throw std::logic_error("QuestCatalog: duplicate registered ID");
            }
            return index;
        }();

        // 外部から渡されたIDを検索し、未登録ならファイル読込前に拒否する。
        const auto entry = filesById.find(id);
        if (entry == filesById.end())
            throw std::invalid_argument("QuestCatalog: unknown quest ID");

        // JSONはキャッシュせず毎回読み直す。Retry時にも変更した設定を反映する。
        auto& files = gameSystem::AppCtx::FileSystem();
        auto config = QuestConfig::FromJson(files.jsonIO.Load(files.GetDataDir() / entry->second));
        if (config.id != id) throw std::runtime_error("QuestCatalog: ID differs from registered file");
        return config;
    }
}
