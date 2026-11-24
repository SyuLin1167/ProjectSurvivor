export module Scene.SceneManager;

import <memory>;
import <unordered_map>;
import <stack>;
import <variant>;
import <vector>;
import GameSystem.FrameRate;

import Asset.Graph;
export import Scene.SceneBase;

/// <summary>
/// シーン関連
/// </summary>
export namespace scene
{
    /// <summary>最上位から不透明シーンまで集め、背面から描く。元の所有スタックは変更しない。</summary>
    /// <param name="scenes">空でない現在のスタックのコピー。</param>
    inline void DrawSceneLayers(std::stack<std::shared_ptr<SceneBase>> scenes)
    {
        std::vector<std::shared_ptr<SceneBase>> layers;
        while (!scenes.empty())
        {
            auto current = scenes.top();
            scenes.pop();
            layers.push_back(current);
            if (!current->DrawsBelow()) break;
        }
        for (auto i = layers.rbegin(); i != layers.rend(); ++i) (*i)->Draw();
    }

    /// <summary>下位シーンを残してPopし、再開時刻をリセットする。</summary>
    /// <param name="scenes">現在の所有スタック。</param>
    inline void PopOverlay(std::stack<std::shared_ptr<SceneBase>>& scenes)
    {
        if (scenes.size() <= 1) return;
        scenes.pop();
        gameSystem::FrameRate::ResetSimulationTime();
    }

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
