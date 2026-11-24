export module Expedition.ExpeditionState;
import Quest.QuestCatalog;
import Progression.ProgressionState;
import <stdexcept>;

export namespace expedition
{
    /// <summary>遠征中だけ存続する値。シーンやゲームオブジェクトへの参照を持たない。</summary>
    struct ExpeditionState
    {
        quest::QuestConfig quest; // 出発時の依頼定義。区間間の再読込でHP効果を再適用しない。
        progression::ProgressionState growth; // Lv・残高・累計XP・保留・4強化の唯一の所有先。
        float health, maxHealth; // 区間境界のHP。Play中の実値はMovingBaseが管理する。
        int section = 1; // 1始まりの区間番号。
        static constexpr int SectionCount = 2; // 今回は同じステージ2区間。将来の構成読込境界。

        /// <summary>検証済み依頼から新規遠征を開始する。継続時はこの初期化を使わない。</summary>
        /// <param name="selected">出発時の依頼。</param>
        /// <param name="config">遠征全体で使う必要XP。</param>
        ExpeditionState(quest::QuestConfig selected, progression::Config config)
            : quest(selected), growth(config), health(selected.initialHealth), maxHealth(selected.maxHealth) {}

        /// <summary>最後の区間かを返す。</summary>
        /// <returns>第2区間ならtrue。</returns>
        bool IsFinalSection() const { return section == SectionCount; }

        /// <summary>中継地点の確定操作で区間を進める。成長・HPは変更しない。</summary>
        void Advance()
        {
            // 誤った遷移による敗北状態の再開や存在しない第3区間を開始境界で拒否する。
            if (section != 1 || health <= 0) throw std::logic_error("Expedition cannot advance from this state");
            ++section;
        }
    };
}
