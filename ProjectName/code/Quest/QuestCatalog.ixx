export module Quest.QuestCatalog;
export import Quest.QuestConfig;
import AppContext;
import MyLib.File.FileSystem;
import <unordered_map>;
import <string_view>;
import <stdexcept>;
import <vector>;

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
        if (config.requiredClear && !AvailableQuests.contains(*config.requiredClear))
            throw std::runtime_error("QuestCatalog: unknown unlock requirement");
        return config;
    }

    /// <summary>保存データのID検証に使う一覧を得る。表示順の管理とは独立。</summary>
    /// <returns>登録済みIDの配列。</returns>
    inline std::vector<std::string> KnownIds()
    {
        std::vector<std::string> ids;
        for (const auto& [id, entry] : AvailableQuests) ids.emplace_back(id);
        return ids;
    }

    /// <summary>定義の解放条件を攻略記録へ照合する。</summary>
    /// <param name="config">検証済み依頼定義。</param>
    /// <param name="progress">読み込んだ攻略記録。</param>
    /// <returns>初期解放か、前提依頼をクリアしていればtrue。</returns>
    inline bool IsUnlocked(const QuestConfig& config, const campaign::CampaignProgress& progress)
    {
        if (!config.requiredClear) return true;
        return progress.Cleared(*config.requiredClear);
    }
}
