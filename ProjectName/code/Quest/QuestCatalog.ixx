export module Quest.QuestCatalog;
export import Quest.QuestConfig;
import AppContext;
import MyLib.File.FileSystem;
import <unordered_map>;
import <string_view>;
import <stdexcept>;

export namespace quest
{
    /// <summary>選択可能なIDと相対JSON名を対応付ける一覧登録。</summary>
    struct CatalogEntry
    {
        const char* file; // resources/dataからの相対パス。
        int displayOrder; // 小さい順に表示し、先頭を初期選択にする。
    };
    inline const std::unordered_map<std::string_view, CatalogEntry> AvailableQuests{
        {"pilgrim", {"Quests/Pilgrim.json", 0}},
        {"cargo", {"Quests/Cargo.json", 1}}
    };

    /// <summary>登録IDを解決し、最新のJSONを検証して読み込む。</summary>
    /// <param name="id">選択画面またはRetryから渡されたID。</param>
    /// <returns>この読込時点の依頼のコピー。</returns>
    inline QuestConfig FindQuest(std::string_view id)
    {
        // 外部から渡されたIDを検索し、未登録ならファイル読込前に拒否する。
        const auto entry = AvailableQuests.find(id);
        if (entry == AvailableQuests.end())
            throw std::invalid_argument("QuestCatalog: unknown quest ID");

        // JSONはキャッシュせず毎回読み直す。Retry時にも変更した設定を反映する。
        auto& files = gameSystem::AppCtx::FileSystem();
        auto config = QuestConfig::FromJson(files.jsonIO.Load(files.GetDataDir() / entry->second.file));
        if (config.id != id) throw std::runtime_error("QuestCatalog: ID differs from registered file");
        return config;
    }
}
