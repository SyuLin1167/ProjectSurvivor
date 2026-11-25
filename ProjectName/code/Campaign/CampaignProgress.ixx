module;
#include <json.hpp>
export module Campaign.CampaignProgress;
import MyLib.File.JsonValue;
import <set>;
import <string>;
import <stdexcept>;

export namespace campaign
{
    /// <summary>遠征外に残す攻略実績。解放状態は依頼定義から導出し、二重保存しない。</summary>
    struct CampaignProgress
    {
        static constexpr int Version = 1; // セーブ形式。将来の版を旧処理で上書きしない。
        std::set<std::string> cleared; // 全区間成功した依頼ID。重複を持たない。

        /// <summary>依頼の全区間成功が記録済みかを調べる。</summary>
        /// <param name="id">依頼ID。</param>
        /// <returns>クリア済みならtrue。</returns>
        bool Cleared(const std::string& id) const { return cleared.contains(id); }

        /// <summary>書き出し用JSONを作る。</summary>
        /// <returns>バージョンとクリア済みIDだけの記録。</returns>
        nlohmann::json ToJson() const { return {{"version", Version}, {"clearedQuests", cleared}}; }

        /// <summary>保存された型・ID一覧を検証する。バージョンは読込側で先に区別する。</summary>
        /// <param name="data">対応バージョンのJSON。</param>
        /// <returns>検証済みの攻略記録。不正な型や重複IDは例外。</returns>
        static CampaignProgress FromJson(const nlohmann::json& data)
        {
            const auto& ids = file::jsonValue::Member(data, "clearedQuests");
            if (!ids.is_array()) throw std::runtime_error("clearedQuests must be an array");
            CampaignProgress result;
            for (const auto& value : ids)
            {
                const auto id = file::jsonValue::Convert<std::string>(value, "clearedQuests[]");
                if (id.empty() || !result.cleared.insert(id).second)
                    throw std::runtime_error("Invalid or duplicate cleared quest ID");
            }
            return result;
        }
    };
}
