export module Scene.Result;
import Scene.SceneBase;
import UI.UIManager;
import Scene.StageState;
import Quest.QuestCatalog;
import Expedition.ExpeditionState;

export namespace scene
{
    /// <summary>ゲーム参照を持たず、確定した結果とRetry・Titleの動線を表示する。</summary>
    class Result final : public SceneBase
    {
    public:
        /// <summary>終了値をコピーし、最小限の結果操作を登録する。</summary>
        /// <param name="result">成功または敗北の確定値。</param>
        /// <param name="state">終了区間・依頼・成長・HPのスナップショット。</param>
        Result(StageResult result, expedition::ExpeditionState state);
        /// <summary>結果は構築時に確定済みのため追加初期化は不要。</summary>
        void Init() override {}
        /// <summary>ゲーム更新をせず、結果画面の操作だけを受け付ける。</summary>
        /// <returns>RetryのLoadingScene、Title、または継続コマンド。</returns>
        SceneCmd Update() override;
        /// @brief 同じ依頼で新しい試行への読み込みを要求する。
        /// @return Retryボタンが使用する遷移コマンド。
        SceneCmd Retry();
        /// <summary>成功・敗北、終了時HP、進行率、操作を画面座標で描画する。</summary>
        void Draw() override;
        /// <summary>引き渡された終了値を返す。</summary>
        /// <returns>ゲームオブジェクト参照を含まない結果のコピー。</returns>
        StageResult ResultValue() const { return m_result; }
        /// <summary>終了した遠征の値を読む。</summary>
        /// <returns>シーン参照を含まない不変の終了状態。</returns>
        const expedition::ExpeditionState& State() const { return m_expedition; }

    private:
        // シーン単位のUI所有。LoadingSceneが旧シーンを保持しても新画面へ混在させない。
        ui::UIManager m_hudManager;
        const expedition::ExpeditionState m_expedition;
        const StageResult m_result;
        bool m_toRetry = false;
        bool m_toTitle = false;
        bool m_leaving = false;
        bool m_saveRequested = false; // 保存失敗時の再試行だけをUI走査後に行う。
    };
}
