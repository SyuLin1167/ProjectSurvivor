export module Scene.Play;
import Scene.SceneBase;
import UI.UIManager;
import MyLib.Math.Vector2;
import Asset.Graph;
import Scene.StageState;
import Object.MovingBase;
import Object.Player;
import Object.EnemySpawner;
import <optional>;

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

        /// <summary>StartとRetry共通の読み込み経路で、新しいPlayへの遷移を要求する。</summary>
        /// <param name="hold">読み込み中の表示を保持する元シーン。</param>
        /// <param name="prototype">既存の試作経路を使用する場合true。</param>
        /// <returns>LoadingSceneへ置き換えるコマンド。</returns>
        static SceneCmd Start(std::shared_ptr<SceneBase> hold, bool prototype = true);

    private:
        // シーン単位のUI所有。LoadingSceneが旧シーンを保持しても新画面へ混在させない。
        ui::UIManager m_hudManager;
        /// <summary>
        /// スクロール背景描画
        /// </summary>
        /// <param name="parallax">視差効果の強さ（0.0～1.0）</param>
        void DrawScrollingBackground(float parallax = 0.5f);

        ui::UIElementID m_hudID{};
        bool m_cleared;     // クリアフラグ
        bool m_prototype;
        std::optional<object::EnemySpawner> m_spawner; // 新規Playごとの生成状態
        std::optional<StageState> m_stage; // 新規Playごとの進行・終了状態
        std::weak_ptr<object::Player> m_player; // HUD用。所有権を延長しない。
        std::weak_ptr<object::MovingBase> m_base; // 所有はObjectManagerに限定
    };
}
