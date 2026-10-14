export module Scene.Result;
import Scene.SceneBase;

/// <summary>
/// シーン関連
/// </summary>
export namespace scene
{
    /// <summary>
    /// リザルトシーンを担当
    /// </summary>
    export class Result final : public SceneBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Result();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Result() = default;

        /// <summary>
        ///　初期化処理
        /// </summary>
        void Init() override {}
        
        /// <summary>
        /// 更新処理
        /// </summary>
        /// <returns>次フレーム適用のコマンド</returns>
        SceneCmd Update() override;

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw() override;
    };
}
