export module Scene.Title;
import Scene.SceneBase;
import UI.UIManager;
import UI.Button;

export namespace scene
{
    /// <summary>攻略記録の継続・新規作成を選ぶ。上書きは確認確定まで行わない。</summary>
    class Title final : public SceneBase
    {
    public:
        /// <summary>初回のみ攻略記録を読み、通常操作と上書き確認UIを構築する。</summary>
        Title();
        /// <summary>タイトル画像と、この画面のUIを解放する。</summary>
        ~Title();
        /// <summary>UIは構築済みのため追加初期化は不要。</summary>
        void Init() override {}
        /// <summary>確認中は確認UIだけ入力を受け付ける。</summary>
        /// <returns>依頼選択・終了・待機コマンド。</returns>
        SceneCmd Update() override;
        /// <summary>タイトル・保存状態・必要なら上書き確認を描画する。</summary>
        void Draw() override;
        /// <summary>記録を再読込して継続する。未保存実績があれば先に保存を再試行する。</summary>
        /// <returns>成功時はQuestSelect、不正記録・保存失敗時は待機。</returns>
        SceneCmd ContinueGame();
        /// <summary>既存記録があれば確認へ進み、不存在なら初期記録を保存する。</summary>
        /// <returns>新規開始完了ならQuestSelect、確認待ちなら待機。</returns>
        SceneCmd RequestNewGame();
        /// <summary>上書き確認へ回答する。取消時はメモリとファイルを変更しない。</summary>
        /// <param name="accepted">新しい攻略記録への置換を確定する場合true。</param>
        /// <returns>保存成功時はQuestSelect、それ以外は待機。</returns>
        SceneCmd ConfirmNewGame(bool accepted);
        /// <summary>確認ダイアログの表示状態を返す。</summary>
        /// <returns>回答待ちならtrue。</returns>
        bool Confirming() const { return m_confirming; }
    private:
        /// <summary>新規開始を保存し、成功時だけ依頼選択へ進む。</summary>
        /// <returns>保存成功時はQuestSelect、失敗時は待機。</returns>
        SceneCmd StartNew();
        /// <summary>通常ボタンの入力履歴を破棄し、継続可否を読込状態に合わせる。</summary>
        void RefreshButtons();
        ui::UIManager m_ui, m_confirmation; // 確認中は通常UIを更新しない。
        ui::Button* m_continueButton; // m_uiが所有し、Titleより長生きしない。
        ui::Button* m_newButton;
        ui::Button* m_exitButton;
        ui::Button* m_acceptButton; // m_confirmationが所有する確認操作。
        ui::Button* m_cancelButton;
        enum class Action { None, Continue, New, Exit, Accept, Cancel };
        Action m_action = Action::None; // UI走査後に処理する要求。
        bool m_confirming = false;
    };
}
