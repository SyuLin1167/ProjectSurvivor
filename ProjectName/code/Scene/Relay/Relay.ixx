export module Scene.Relay;
import Scene.SceneBase;
import UI.UIManager;
import Expedition.ExpeditionState;
import UI.Button;
import <optional>;
import <string>;

export namespace scene
{
    /// <summary>区間間の操作待ち画面。遠征の値と専用UIだけを所有し、戦闘を更新しない。</summary>
    class Relay final : public SceneBase
    {
    public:
        /// <summary>成功区間の遠征値を受け取り、次区間へ進むUIを生成する。</summary>
        /// <param name="state">第1区間成功時の成長とHP。</param>
        explicit Relay(expedition::ExpeditionState state);
        /// <summary>UIは構築時に登録済み。</summary>
        void Init() override {}
        /// <summary>確定操作だけを処理し、自動では区間を進めない。</summary>
        /// <returns>継続・LoadingScene・Titleへの遷移。</returns>
        SceneCmd Update() override;
        /// <summary>区間番号・HP・Lvと操作を表示する。</summary>
        void Draw() override;
        /// <summary>次区間をLoadingScene経由で開始する。二重操作は無視する。</summary>
        /// <returns>初回のみLoadingSceneへの遷移。</returns>
        SceneCmd Continue();
        /// <summary>接続先だけ選択する。選択だけではHPや遠征位置を変更しない。</summary>
        /// <param name="id">現在地点から到達できるノードID。</param>
        /// <returns>選択を受理した場合true。無効IDや遷移後はfalse。</returns>
        bool SelectRoute(const std::string& id);
        /// <summary>ゲームオブジェクト参照を含まない遠征値を読む。</summary>
        /// <returns>中継地点が保持する不変の遠征状態。</returns>
        const expedition::ExpeditionState& State() const { return m_state; }
    private:
        const expedition::ExpeditionState m_state;
        ui::UIManager m_ui;
        std::optional<std::string> m_selected; // UI上の仮選択。出発確定まで遠征値へ反映しない。
        ui::Button* m_depart; // m_ui所有。構築時に必ず生成し、シーンより長生きしない。
        bool m_continue = false, m_title = false, m_leaving = false;
    };
}
