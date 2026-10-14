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
        Play();

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
    };
}
