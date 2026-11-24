export module Scene.LevelUp;
import Scene.SceneBase;
import UI.UIManager;
import Progression.ProgressionState;
import <optional>;
export namespace scene
{
    /// <summary>Playを背景に保持し、強化の選択入力だけを受け付けるオーバーレイ。</summary>
    class LevelUp final : public SceneBase
    {
        progression::ProgressionState& m_growth; // スタック下位のPlay所有。Popまで存続する。
        ui::UIManager m_ui;
        std::optional<progression::Upgrade> m_choice;
    public:
        /// <summary>Playの成長状態から3つのボタンを生成する。</summary>
        /// <param name="growth">下位Playの試行内状態。</param>
        explicit LevelUp(progression::ProgressionState& growth);
        /// <summary>背景シーンを背面に描く。</summary>
        /// <returns>常にtrue。</returns>
        bool DrawsBelow() const override { return true; }
        /// <summary>生成済みUI以外の初期化は不要。</summary>
        void Init() override {}
        /// <summary>選択を確定した場合だけPopする。</summary>
        /// <returns>Popまたは継続。</returns>
        SceneCmd Update() override;
        /// <summary>半透明背景と3候補を描く。</summary>
        void Draw() override;
    };
}
