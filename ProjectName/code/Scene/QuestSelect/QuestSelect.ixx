export module Scene.QuestSelect;
import Scene.SceneBase;
import UI.UIManager;
import Quest.QuestCatalog;
import <vector>;
import <string>;

export namespace scene
{
    /// <summary>依頼一覧と説明を所有し、選択したIDだけを出発経路へ渡す。</summary>
    class QuestSelect final : public SceneBase
    {
    public:
        /// <summary>登録済み依頼を読み込み、この画面専用の操作を作る。</summary>
        QuestSelect();
        /// <summary>追加初期化は不要。</summary>
        void Init() override {}
        /// <summary>選択・出発・戻るを受け付ける。読込失敗は安全に終了する。</summary>
        /// <returns>Loading、Title、終了または継続コマンド。</returns>
        SceneCmd Update() override;
        /// <summary>一覧と選択中依頼の効果を描画する。</summary>
        void Draw() override;
    private:
        ui::UIManager m_ui; // ボタンはシーンと同時に破棄する。
        std::vector<quest::QuestConfig> m_quests;
        size_t m_selected = 0;
        bool m_depart = false, m_back = false, m_failed = false;
    };
}
