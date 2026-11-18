export module Quest.QuestCatalog;
import <array>;
import <stdexcept>;

export namespace quest
{
    /// @brief 同じ目的地で選べる護送依頼の識別子。
    enum class Id { Pilgrim, Cargo };

    /// @brief 出発時にコピーする不変の依頼定義。文字列の寿命はプログラム全体。
    struct QuestDefinition
    {
        Id id;
        const char* name;
        const char* description;
        int healthBonus; // 基準HPへの加減算値。
    };

    /// @brief タイトルで選択できる依頼を表示順に返す。
    /// @return 名前・説明・効果を含む定義一覧。
    inline constexpr std::array AvailableQuests{
        QuestDefinition{Id::Pilgrim, "巡礼者の護送", "探索にも余裕のある安全な護送", 20},
        QuestDefinition{Id::Cargo, "魔導物資の緊急輸送", "防衛を優先したい危険な輸送", -20}
    };

    /// @brief 出発する依頼を確定し、未知のIDは拒否する。
    /// @param id 選択された依頼ID。
    /// @return 該当する依頼の定義。
    inline QuestDefinition FindQuest(Id id)
    {
        for (const auto& entry : AvailableQuests)
            if (entry.id == id) return entry;
        throw std::invalid_argument("Unknown escort quest");
    }
}
