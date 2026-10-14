export module Scene.Title;
import Scene.SceneBase;
import MyLib.Math.Vector2;

using namespace math;

/// <summary>
/// シーン関連
/// </summary>
export namespace scene
{
    /// <summary>
    /// タイトルシーンを担当
    /// </summary>
    export class Title final :public SceneBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Title();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Title();

        /// <summary>
        /// 初期化処理
        /// </summary>
        void Init() override {};

        /// <summary>
        /// 更新処理
        /// </summary>
        SceneCmd Update() override;

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw() override;

        /// <summary>
        /// ローディング画面描画処理
        /// </summary>
        void DrawLoading() override;

    private:
        static constexpr Vector2f START_BTN_POS = { 900.0f, 900.0f };     // スタートボタン位置
        static constexpr Vector2f EXIT_BTN_POS  = { 900.0f, 980.0f };     // Exitボタン位置
        static constexpr Vector2f BTN_SIZE      = { 120.0f, 50.0f };      // ボタンサイズ
        static constexpr Vector2f PROGRESS_TEXT_POS = { 1600.0f, 1050.0f }; // プログレステキスト位置

        bool m_toNextScene = false;         // 次シーンへ移動フラグ
        bool m_toExit = false;              // 終了フラグ
        ui::UIElementID m_startButtonID;    // スタートボタンID
        ui::UIElementID m_exitButtonID;     // ExitボタンID
    };
}

