export module Scene.SceneBase;
export import <memory>;
export import <functional>;
export import <variant>;
export import Object.ObjectManager;
export import AppContext;

/// <summary>
/// シーン関連
/// </summary>
export namespace scene
{
    export class SceneBase; // 前方宣言

    export using SceneBuilder = std::function<std::shared_ptr<SceneBase>()>;    // シーン構築関数

    export struct CmdPush { SceneBuilder build; };      // シーンを積む
    export struct CmdReplace { SceneBuilder build; };   // シーンを置き換える
    export struct CmdPop {};                            // シーンを削除する
    export struct CmdQuit {};                           // 終了要求

    export using SceneCmd = std::variant<std::monostate, CmdPush, CmdReplace, CmdPop, CmdQuit>; // シーンコマンド

    /// <summary>
    /// シーンの基底クラス
    /// </summary>
    export class SceneBase : public std::enable_shared_from_this<SceneBase>
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        SceneBase()
        {
            m_objManager = std::make_unique<object::ObjectManager>();
        }

        /// <summary>
        /// デストラクタ
        /// </summary>
        virtual ~SceneBase() = default;

        /// <summary>
        /// 初期化処理
        /// </summary>
        virtual void Init() = 0;

        /// <summary>
        /// 更新処理
        /// </summary>
        virtual SceneCmd Update() = 0;

        /// <summary>
        /// 描画処理
        /// </summary>
        virtual void Draw() = 0;

        /// <summary>
        /// ローディング画面描画処理
        /// </summary>
        virtual void DrawLoading() {};

    protected:
        std::unique_ptr<object::ObjectManager> m_objManager;        //オブジェクト管理のインスタンス
    };
}