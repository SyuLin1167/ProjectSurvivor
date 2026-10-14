export module GameSystem.Execution;

import <memory>;

// 前方宣言
export namespace scene
{
    class SceneManager;
}

/// <summary>
/// システム関連
/// </summary>
export namespace gameSystem
{
    class Library;

    /// <summary>
    /// ゲームの実行を行う
    /// </summary>
    export class Execution final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Execution();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Execution();

        /// <summary>
        /// ゲーム実行処理
        /// </summary>
        /// <returns>-1:エラー|0:正常終了</returns>
        int Run();

    private:
        std::unique_ptr<class Library> m_library;         //ライブラリ管理のインスタンス
        std::unique_ptr<scene::SceneManager> m_scene;     //シーン管理のインスタンス
    };
}
