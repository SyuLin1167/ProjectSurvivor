export module Scene.SceneManager;

import <memory>;
import <unordered_map>;
import <stack>;
import <variant>;

import Asset.Graph;
export import Scene.SceneBase;

/// <summary>
/// シーン関連
/// </summary>
export namespace scene
{
    // 前方宣言
    export class SceneBase;

    /// <summary>
    /// シーンを管理する
    /// </summary>
    export class SceneManager final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        SceneManager();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~SceneManager() = default;

        /// <summary>
        /// シーンのループ処理
        /// </summary>
        void GameLoop();

    private:
        /// <summary>
        /// シーンの更新処理
        /// </summary>
        void Update();

        /// <summary>
        /// シーンの描画処理
        /// </summary>
        void Draw();

        /// <summary>
        /// シーンの切り替え処理
        /// </summary>
        void ChangeScene();

        std::stack<std::shared_ptr<SceneBase>> m_nowScene;  //現在のシーン
        SceneCmd m_pendingCmd;                              //次フレームに適用するコマンド
        bool m_isRunning;                                   //ゲームループ継続フラグ
    };
}
