export module Scene.Result;
import Scene.SceneBase;
import Scene.StageState;

export namespace scene
{
    /// <summary>ゲーム参照を持たず、確定した結果とRetry・Titleの動線を表示する。</summary>
    class Result final : public SceneBase
    {
    public:
        /// <summary>終了値をコピーし、最小限の結果操作を登録する。</summary>
        /// <param name="result">成功または敗北の確定値。</param>
        /// <param name="retryPrototype">再試行時に既存試作経路を使う場合true。</param>
        explicit Result(StageResult result, bool retryPrototype = true);
        /// <summary>このシーンが登録したボタンを破棄する。</summary>
        ~Result();
        /// <summary>結果は構築時に確定済みのため追加初期化は不要。</summary>
        void Init() override {}
        /// <summary>ゲーム更新をせず、結果画面の操作だけを受け付ける。</summary>
        /// <returns>RetryのLoadingScene、Title、または継続コマンド。</returns>
        SceneCmd Update() override;
        /// <summary>成功・敗北、終了時HP、進行率、操作を画面座標で描画する。</summary>
        void Draw() override;
        /// <summary>引き渡された終了値を返す。</summary>
        /// <returns>ゲームオブジェクト参照を含まない結果のコピー。</returns>
        StageResult ResultValue() const { return m_result; }

    private:
        const StageResult m_result;
        bool m_retryPrototype;
        bool m_toRetry = false;
        bool m_toTitle = false;
        bool m_leaving = false;
        ui::UIElementID m_retryButtonID{};
        ui::UIElementID m_titleButtonID{};
    };
}
