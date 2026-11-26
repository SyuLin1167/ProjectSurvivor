export module Progression.ProgressionState;
import MyLib.File.JsonValue;
import MyLib.File.JsonIO;
import <array>;
import <cstdint>;
import <stdexcept>;
import <limits>;
import <algorithm>;

export namespace progression
{
    inline constexpr int MaxLevel = 99; // 遠征内の上限。HUDと選択回数もこの値を参照する。
    /// <summary>試行内だけ有効な4種類の強化。</summary>
    enum class Upgrade { Damage, AttackSpeed, Movement, Magnet };
    /// <summary>Lv2〜5の必要XPと、その後の線形増加量。外部設定は開始時だけ検証する。</summary>
    struct Config
    {
        std::array<uint64_t, 4> required{3,4,5,6};
        uint64_t xpIncreasePerLevel = 1; // Lv5以降の次レベル必要XPに毎段階加える量。0なら固定。
        /// <summary>序盤4値と増加量を読む。旧JSONで増加量を省略した場合は1を採用する。</summary>
        /// <param name="data">ProgressionConfigのJSON。</param>
        /// <returns>検証済み必要XP。</returns>
        static Config FromJson(const json& data)
        {
            const auto& values = file::jsonValue::Member(data, "requiredXp");
            if (!values.is_array() || values.size() != 4) throw std::runtime_error("Progression requires four XP thresholds");
            Config config;
            for (size_t i = 0; i < 4; ++i)
            {
                const auto amount = file::jsonValue::Convert<double>(values[i], "requiredXp[]");
                if (!values[i].is_number_integer() || amount <= 0 || amount > 1000000)
                    throw std::runtime_error("Progression XP must be positive integers <= 1000000");
                config.required[i] = file::jsonValue::Convert<uint64_t>(values[i], "requiredXp[]");
            }
            const auto increase = file::jsonValue::ReadOr<json>(data, "xpIncreasePerLevel", json(1));
            const auto amount = file::jsonValue::Convert<double>(increase, "xpIncreasePerLevel");
            if (!increase.is_number_integer() || amount < 0 || amount > 1000000)
                throw std::runtime_error("Progression xpIncreasePerLevel must be an integer in 0..1000000");
            config.xpIncreasePerLevel = file::jsonValue::Convert<uint64_t>(increase, "xpIncreasePerLevel");
            return config;
        }
    };
    /// <summary>経験値残高・累計・レベル・未選択回数・強化回数を遠征の寿命で所有する。</summary>
    class ProgressionState
    {
        Config m_config;
        uint64_t m_xp = 0; // 次のレベルへ消費する残高。Lv99では余剰XP。
        uint64_t m_totalXp = 0; // 区間を跨ぐ取得累計。レベルアップで消費しない。
        int m_level = 1, m_pending = 0, m_selected = 0;
        std::array<int, 4> m_ranks{}; // 個別の取得上限は設けない。遠征全体で最大98選択なので枯渇しない。
    public:
        /// <summary>検証済み設定で新規試行を開始する。</summary>
        /// <param name="config">必要XP。</param>
        explicit ProgressionState(Config config) : m_config(config) {}
        /// <summary>今回取得分を蓄積し、複数レベルの選択を保留する。</summary>
        /// <param name="amount">未加算の取得XP。</param>
        void AddExperience(uint64_t amount)
        {
            // 加算前に残り容量を使い、極端な取得量でも桁あふれで巻き戻らない。
            // Lv99以降も累計・余剰XPは記録するが、新しい選択は発生させない。
            const auto maximum = (std::numeric_limits<uint64_t>::max)();
            m_xp += (std::min)(amount, maximum - m_xp);
            m_totalXp += (std::min)(amount, maximum - m_totalXp);
            while (m_level < MaxLevel && m_xp >= NextRequired())
            {
                m_xp -= NextRequired();
                ++m_level;
                ++m_pending;
            }
        }
        /// <summary>選択回数ごとに開始位置をずらす固定巡回で、異なる3候補を提示する。</summary>
        /// <returns>表示順の異なる3候補。4種類とも再取得可能なので、98回の選択中も枯渇しない。</returns>
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
        /// <returns>1〜99。</returns>
        int Level() const { return m_level; }
        /// <summary>未選択の回数を返す。</summary>
        /// <returns>0〜98。</returns>
        int Pending() const { return m_pending; }
        /// <summary>次レベル用の残高を返す。</summary>
        /// <returns>消費後XP。</returns>
        uint64_t Xp() const { return m_xp; }
        /// <summary>遠征全体での取得累計を返す。</summary>
        /// <returns>消費前の累計XP。</returns>
        uint64_t TotalXp() const { return m_totalXp; }
        /// <summary>次の必要XPを返す。</summary>
        /// <returns>Lv99では0。それ以外は現在レベルから次への必要XP。</returns>
        uint64_t NextRequired() const
        {
            if (m_level == MaxLevel) return 0;
            const auto index = static_cast<size_t>(m_level - 1);
            if (index < m_config.required.size()) return m_config.required[index];
            // 既存4値の最後を起点とする。仮値ではLv5→6が7、Lv98→99が100。
            return m_config.required.back() +
                uint64_t(index - m_config.required.size() + 1) * m_config.xpIncreasePerLevel;
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
