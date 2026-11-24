export module Progression.ProgressionState;
import MyLib.File.JsonIO;
import <array>;
import <cstdint>;
import <stdexcept>;

export namespace progression
{
    /// <summary>試行内だけ有効な4種類の強化。</summary>
    enum class Upgrade { Damage, AttackSpeed, Movement, Magnet };
    /// <summary>Lv2〜5までの追加必要XP。外部設定は開始時だけ検証する。</summary>
    struct Config
    {
        std::array<uint64_t, 4> required{3,4,5,6};
        /// <summary>4つの正の整数を読み、到達不能な設定を拒否する。</summary>
        /// <param name="data">ProgressionConfigのJSON。</param>
        /// <returns>検証済み必要XP。</returns>
        static Config FromJson(const json& data)
        {
            const auto& values = data.at("requiredXp");
            if (!values.is_array() || values.size() != 4) throw std::runtime_error("Progression requires four XP thresholds");
            Config config;
            for (size_t i = 0; i < 4; ++i)
            {
                if (!values[i].is_number_integer() || values[i].get<double>() <= 0 || values[i].get<double>() > 1000000)
                    throw std::runtime_error("Progression XP must be positive integers <= 1000000");
                config.required[i] = values[i].get<uint64_t>();
            }
            return config;
        }
    };
    /// <summary>経験値残高・累計・レベル・未選択回数・強化回数を遠征の寿命で所有する。</summary>
    class ProgressionState
    {
        Config m_config;
        uint64_t m_xp = 0; // 次のレベルへ消費する残高。Lv5では余剰XP。
        uint64_t m_totalXp = 0; // 区間を跨ぐ取得累計。レベルアップで消費しない。
        int m_level = 1, m_pending = 0, m_selected = 0;
        std::array<int, 4> m_ranks{}; // 各4回まで。全試行の選択自体も最大4回。
    public:
        /// <summary>検証済み設定で新規試行を開始する。</summary>
        /// <param name="config">必要XP。</param>
        explicit ProgressionState(Config config) : m_config(config) {}
        /// <summary>今回取得分を蓄積し、複数レベルの選択を保留する。</summary>
        /// <param name="amount">未加算の取得XP。</param>
        void AddExperience(uint64_t amount)
        {
            m_xp += amount;
            m_totalXp += amount;
            while (m_level < 5 && m_xp >= NextRequired())
            {
                m_xp -= NextRequired();
                ++m_level;
                ++m_pending;
            }
        }
        /// <summary>選択回数ごとに開始位置をずらす固定巡回で、異なる3候補を提示する。</summary>
        /// <returns>表示順の候補。最大4選択なので、選択待ち中は必ず3種類以上選択可能。</returns>
        std::array<Upgrade, 3> Choices() const
        {
            std::array<Upgrade, 3> result{};
            for (int i = 0; i < 3; ++i) result[i] = static_cast<Upgrade>((m_selected + i) % 4);
            return result;
        }
        /// <summary>提示候補から選び、一つの保留を消費する。古い入力の再適用を拒否する。</summary>
        /// <param name="choice">選択された強化。</param>
        /// <returns>適用された場合true。</returns>
        bool Select(Upgrade choice)
        {
            if (m_pending == 0) return false;
            bool available = false;
            for (auto candidate : Choices()) if (candidate == choice) available = true;
            if (!available) return false;
            ++m_ranks[static_cast<size_t>(choice)];
            --m_pending;
            ++m_selected;
            return true;
        }
        /// <summary>現在レベルを返す。</summary>
        /// <returns>1〜5。</returns>
        int Level() const { return m_level; }
        /// <summary>未選択の回数を返す。</summary>
        /// <returns>0〜4。</returns>
        int Pending() const { return m_pending; }
        /// <summary>次レベル用の残高を返す。</summary>
        /// <returns>消費後XP。</returns>
        uint64_t Xp() const { return m_xp; }
        /// <summary>遠征全体での取得累計を返す。</summary>
        /// <returns>消費前の累計XP。</returns>
        uint64_t TotalXp() const { return m_totalXp; }
        /// <summary>次の必要XPを返す。</summary>
        /// <returns>Lv5では0。</returns>
        uint64_t NextRequired() const
        {
            if (m_level == 5) return 0;
            return m_config.required[m_level-1];
        }
        /// <summary>各強化の加算倍率を返す。</summary>
        /// <param name="upgrade">能力の種類。</param>
        /// <returns>基礎値に掛ける倍率。攻撃速度のみ間隔の除数。</returns>
        float Multiplier(Upgrade upgrade) const
        {
            const std::array<float,4> increments{0.20f,0.15f,0.10f,0.25f};
            const auto index = static_cast<size_t>(upgrade);
            return 1 + increments[index] * m_ranks[index];
        }
    };
}
