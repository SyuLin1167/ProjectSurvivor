export module Scene.LoadingScene;

import <typeindex>;

import Scene.SceneBase;
import MyLib.Coroutine;

// 前方宣言
export namespace task
{
    class Loading;
}

/// <summary>
/// シーン関連
/// </summary>
export namespace scene
{
    /// <summary>
    /// ローディング規約
    /// </summary>
    export enum LoadPolicy :int8_t
    {
        NONE = 0,               // なし
        COROUTINE = 1 << 0,     // コルーチン
        PROGRESS = 1 << 1,      // 進捗表示
        DONE = 1 << 2,          // 結果のみ
    };

    /// <summary>
    /// ローディング画面を担当
    /// </summary>
    export class LoadingScene final :public SceneBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        LoadingScene() = default;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="policy">ポリシー</param>
        /// <param name="nextScene">次シーンを返す関数</param>
        /// <param name="holdScene">読み込み中の描画用に保持する元シーン。</param>
        LoadingScene(int8_t policy, std::function<std::shared_ptr<SceneBase>()> nextScene, std::shared_ptr<SceneBase> holdScene = nullptr);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~LoadingScene();

        /// <summary>
        /// 初期化処理
        /// </summary>
        void Init() override {};

        /// <summary>
        /// コルーチン読み込み
        /// </summary>
        /// <returns>生成後のコルーチン</returns>
        Coroutine LoadCoroutine();

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <returns>完了・失敗時の遷移要求。読み込み中は空のコマンド。</returns>
        SceneCmd Update() override;

        /// <summary>
        /// 読み込み失敗時の遷移先
        /// </summary>
        /// <returns>タイトル復帰または終了要求。</returns>
        SceneCmd OnLoadFailed();

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw() override;

    private:
        /// <summary>次シーン生成の例外を読み込み失敗として記録する。</summary>
        /// <returns>次シーンを生成できた場合true。</returns>
        bool BuildNextScene();
        uint8_t m_policy;                                 //ポリシー
        std::shared_ptr<SceneBase> m_holdScene;           //一時確保用のシーン
        std::shared_ptr<SceneBase> m_nextScene;           //次シーン
        std::function<std::shared_ptr<SceneBase>()> m_loadScene;  //次シーン読み込み処理
        Coroutine m_coroutine;                            //コルーチン
        float m_waitElapsed = 0.0f; // ロードごとの待機秒数
        bool m_failed = false;                            //読み込み失敗
    };
}
