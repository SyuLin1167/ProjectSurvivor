export module Expedition.ExpeditionState;
import Quest.QuestCatalog;
import Object.EnemySpawner;
import MyLib.File.JsonIO;
import Expedition.ExpeditionMap;
import Progression.ProgressionState;
import <stdexcept>;
import <algorithm>;
import <string>;

export namespace expedition
{
    /// <summary>遠征中だけ存続する値。シーンやゲームオブジェクトへの参照を持たない。</summary>
    struct ExpeditionState
    {
        quest::QuestConfig quest; // 出発時の依頼定義。区間間でHP効果を再適用しない。
        progression::ProgressionState growth; // Lv・XP・保留・4強化の唯一の所有先。
        float health, maxHealth; // 区間境界のHP。Play中の実値はMovingBaseが管理する。

        /// <summary>検証済み依頼から新規遠征を開始し、ルートも開始地点へ戻す。</summary>
        /// <param name="selected">出発時の依頼。</param>
        /// <param name="config">遠征全体で使う必要XP。</param>
        ExpeditionState(quest::QuestConfig selected, progression::Config config)
            : quest(selected), growth(config), health(selected.initialHealth), maxHealth(selected.maxHealth),
              m_currentNode(DefaultMap().Start()) {}

        /// <summary>現在位置と区間効果の唯一の定義を返す。成長状態へ効果を混ぜない。</summary>
        /// <returns>読み取り専用マップ内の現在ノード。</returns>
        const MapNode& CurrentNode() const { return DefaultMap().Find(m_currentNode); }
        /// <summary>表示用の区間番号を現在ノードから導出する。</summary>
        /// <returns>1始まりの区間番号。</returns>
        int Section() const { return CurrentNode().layer + 1; }
        /// <summary>固定試作の全区間数をマップから導出する。</summary>
        /// <returns>最深階層までの区間数。</returns>
        static int SectionCount() { return DefaultMap().SectionCount(); }
        /// <summary>後続接続がない場合に遠征を完了する。</summary>
        /// <returns>終端ノードならtrue。</returns>
        bool IsFinalSection() const { return CurrentNode().next.empty(); }

        /// <summary>接続先を選択して進入報酬を一度適用する。不正な選択では状態を変更しない。</summary>
        /// <param name="target">現在ノードの接続先ID。</param>
        void Advance(const std::string& target)
        {
            const auto& links = CurrentNode().next;
            // 開始境界で接続と生存を保証する。終端からの再選択・二重報酬も拒否する。
            if (health <= 0 || std::find(links.begin(), links.end(), target) == links.end())
                throw std::logic_error("Expedition: selected node is not an available connection");
            const auto& next = DefaultMap().Find(target);
            m_currentNode = next.id;
            health = (std::min)(maxHealth, health + next.route.entryHeal);
        }

        /// <summary>依頼設定へ区間の上限を反映する。比較用classicの意味は変更しない。</summary>
        /// <param name="source">既存の基本EnemySpawnConfig JSON。</param>
        /// <returns>生成器へ渡す検証済み設定。周期・候補・個体性能は維持する。</returns>
        object::EnemySpawnConfig BuildSpawnConfig(const json& source) const
        {
            auto config = quest.BuildSpawnConfig(source);
            const auto& route = CurrentNode().route;
            if (config.offscreen && route.enemyLimit) config.limit = *route.enemyLimit;
            // 依頼設定はBuildSpawnConfig、ルート上限10/12はMap構築時に検証済み。
            // offscreenの上限だけを置き換えても他の制約は変わらないため、同じ検証を再実行しない。
            return config;
        }

    private:
        std::string m_currentNode; // 選択済みルートのID。区間番号や効果を重複保存しない。
    };
}