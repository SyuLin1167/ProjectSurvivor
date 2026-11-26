export module Expedition.ExpeditionMap;
import <vector>;
import <string>;
import <optional>;
import <algorithm>;
import <iterator>;
import <stdexcept>;
import <utility>;
import <cmath>;

export namespace expedition
{
    /// <summary>マップ上の区間種別。現試作では戦闘のみを扱う。</summary>
    enum class NodeKind { Battle };

    /// <summary>進入時報酬と、そのノード内だけで使う戦闘条件。成長・アイテム状態は持たない。</summary>
    struct RouteConditions
    {
        std::optional<int> enemyLimit; // 画面外生成の最終段階上限。未指定は依頼の値。
        float entryHeal = 0; // 進入時に一度だけ加えるHP。
        float damageMultiplier = 1; // 基礎攻撃力と成長の積に掛ける区間倍率。
    };

    /// <summary>ノードの不変定義。IDは配列添字ではなく、接続はIDで指定する。</summary>
    struct MapNode
    {
        std::string id;
        NodeKind kind = NodeKind::Battle;
        int layer = 0; // 0始まりの区間階層。同階層への接続は禁止。
        std::vector<std::string> next;
        std::string name, description; // 既存UIへ渡す表示文字列。
        RouteConditions route;
    };

    /// <summary>固定DAGの定義と整合性を所有する。遠征ごとの現在位置やHPは持たない。</summary>
    class ExpeditionMap
    {
    public:
        /// <summary>全定義を一度検証し、その後は読み取り専用として提供する。</summary>
        /// <param name="nodes">ID・接続・階層・条件の一覧。並び順は選択UIと独立。</param>
        /// <param name="start">遠征開始ノードのID。</param>
        ExpeditionMap(std::vector<MapNode> nodes, std::string start)
            : m_nodes(std::move(nodes)), m_start(std::move(start))
        {
            // IDの一意性と条件を先に保証し、接続先検索の意味を確定する。
            for (size_t i = 0; i < m_nodes.size(); ++i)
            {
                const auto& node = m_nodes[i];
                if (node.id.empty() || node.name.empty() || node.kind != NodeKind::Battle || node.layer < 0 || size_t(node.layer) >= m_nodes.size())
                    throw std::runtime_error("ExpeditionMap: invalid node identity or layer");
                for (size_t j = 0; j < i; ++j)
                    if (m_nodes[j].id == node.id) throw std::runtime_error("ExpeditionMap: duplicate node ID");
                const auto& route = node.route;
                if ((route.enemyLimit && *route.enemyLimit != 10 && *route.enemyLimit != 12) ||
                    !std::isfinite(route.entryHeal) || route.entryHeal < 0 ||
                    !std::isfinite(route.damageMultiplier) || route.damageMultiplier <= 0)
                    throw std::runtime_error("ExpeditionMap: invalid route conditions");
                m_sectionCount = (std::max)(m_sectionCount, node.layer + 1);
            }
            if (Find(m_start).layer != 0) throw std::runtime_error("ExpeditionMap: start must be layer zero");

            // 接続は次階層へ限定する。循環・逆走・自己接続はこの不変条件で成立しない。
            // 別の親が同じ次ノードを参照する合流は許す。
            for (const auto& node : m_nodes)
            {
                for (size_t i = 0; i < node.next.size(); ++i)
                {
                    if (Find(node.next[i]).layer != node.layer + 1)
                        throw std::runtime_error("ExpeditionMap: edge must advance one layer (cycles forbidden)");
                    for (size_t j = 0; j < i; ++j)
                        if (node.next[j] == node.next[i]) throw std::runtime_error("ExpeditionMap: duplicate edge");
                }
            }

            // 未接続の定義を黙って捨てず、開始地点から到達可能であることを構築時に保証する。
            std::vector<std::string> reached{m_start};
            for (size_t i = 0; i < reached.size(); ++i)
                for (const auto& id : Find(reached[i]).next)
                    if (std::find(reached.begin(), reached.end(), id) == reached.end()) reached.push_back(id);
            if (reached.size() != m_nodes.size()) throw std::runtime_error("ExpeditionMap: unreachable node");
        }

        /// <summary>IDから定義を取得する。外部からの不正な選択もここで拒否する。</summary>
        /// <param name="id">配列添字と無関係なノードID。</param>
        /// <returns>マップの存続期間に有効な定義参照。不明IDはatがout_of_rangeを通知する。</returns>
        const MapNode& Find(const std::string& id) const
        {
            const auto found = std::find_if(m_nodes.begin(), m_nodes.end(),
                [&id](const MapNode& node) { return node.id == id; });
            // 未発見ならindexはsize()になる。境界検証はatに任せ、endの参照や独自例外を重ねない。
            const auto index = static_cast<size_t>(std::distance(m_nodes.begin(), found));
            return m_nodes.at(index);
        }

        /// <summary>新規遠征の開始地点を返す。</summary>
        /// <returns>開始ノードID。</returns>
        const std::string& Start() const { return m_start; }
        /// <summary>表示用の全区間数を返す。</summary>
        /// <returns>最深階層までの区間数。</returns>
        int SectionCount() const { return m_sectionCount; }

    private:
        std::vector<MapNode> m_nodes; // 構築後に変更せず、参照の有効性を保つ。
        std::string m_start;
        int m_sectionCount = 0;
    };

    /// <summary>2区間試作の読み取り専用定義。接続と仮値の追加・調整箇所を一か所に限定する。</summary>
    /// <returns>アプリ存続中に有効な検証済み固定マップ。遠征状態は共有しない。</returns>
    inline const ExpeditionMap& DefaultMap()
    {
        static const ExpeditionMap map({
            {"departure", NodeKind::Battle, 0, {"safe", "danger"}, "出発", "通常戦闘", {}},
            {"safe", NodeKind::Battle, 1, {}, "安全ルート", "低危険度 / 上限10体 / 進入時HP+20", {10, 20, 1}},
            {"danger", NodeKind::Battle, 1, {}, "危険ルート", "高危険度 / 上限12体 / 攻撃+25% / 回復なし", {12, 0, 1.25f}}
        }, "departure");
        return map;
    }
}