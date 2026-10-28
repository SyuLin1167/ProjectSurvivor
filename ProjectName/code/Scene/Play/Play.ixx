export module Scene.Play;
import Scene.SceneBase;
import MyLib.Math.Vector2;
import Asset.Graph;

/// <summary>
/// シーン関連
/// </summary>
export namespace scene
{
    /// <summary>
    /// プレイシーンを担当
    /// </summary>
    export class Play final :public SceneBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="prototype">既存の試作経路を使う場合true。旧サンプル経路はfalse。</param>
        explicit Play(bool prototype = true);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Play();

        /// <summary>
        /// 初期化処理
        /// </summary>
        void Init() override;

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <returns>更新後の遷移要求。継続時は空のコマンド。</returns>
        SceneCmd Update() override;

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw() override;

    private:
        /// <summary>
        /// スクロール背景描画
        /// </summary>
        /// <param name="parallax">視差効果の強さ（0.0～1.0）</param>
        void DrawScrollingBackground(float parallax = 0.5f);

        bool m_cleared;     // クリアフラグ
        bool m_prototype;
        bool m_trialFinished = false; // Phase 1試行の終端。正式な勝敗とは分離
    };
}
