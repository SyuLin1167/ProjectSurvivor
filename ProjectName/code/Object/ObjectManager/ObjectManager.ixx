export module Object.ObjectManager;
import Object.GameObject;
import <memory>;
import <string>;
import <vector>;
import <unordered_map>;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    /// <summary>
    /// オブジェクトを管理する
    /// </summary>
    export class ObjectManager final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        ObjectManager();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~ObjectManager();
        /// <summary>シーンに属する一時アイテムの接触を通常のStepに登録する。</summary>
        /// <returns>このシーンが所有する衝突管理。</returns>
        col2d::ColliderManager& Colliders() { return m_colMgr; }

        /// <summary>
        /// オブジェクト追加
        /// </summary>
        /// <param name="object">追加するオブジェクト</param>
        template<typename... Args>
        inline void AddObject(GameObject* object, Args&&... args)
        {
            QueueOrRegister(std::shared_ptr<GameObject>(object, std::forward<Args>(args)...));
        }

        /// <summary>更新走査の外で新しい個体だけを参照解決・初期化し、所有する。共有所有権とカスタム解放処理を保持する。</summary>
        /// <param name="object">未登録・未初期化の個体。失敗時は渡された所有参照を解放する。</param>
        void AddInitializedObject(std::shared_ptr<GameObject> object);

        /// <summary>
        /// オブジェクト除去
        /// </summary>
        /// <param name="object">除去するオブジェクト</param>
        void RemoveObject(GameObject* object);

        /// <summary>
        /// 更新処理前の初期化
        /// </summary>
        void InitBeforeUpdate();

        /// <summary>
        /// オブジェクト更新
        /// </summary>
        void Update();

        /// <summary>
        /// オブジェクト配列への反映
        /// </summary>
        void RefreshObjects();

        /// <summary>
        /// オブジェクト取得
        /// </summary>
        /// <param name="tag">タグ</param>
        /// <returns>タグに該当するオブジェクト群</returns>
        std::vector<std::shared_ptr<GameObject>> GetObjForTag(uint32_t tag);

        /// <summary>
        /// オブジェクト描画
        /// </summary>
        void Draw();

    private:
        /// @brief 所有権を受け取り、更新中の登録だけを保留する。
        /// @param object 解放方法を保持する所有参照。
        void QueueOrRegister(std::shared_ptr<GameObject> object);
        /// @brief 空参照を拒否し、このシーンの衝突管理に所属させる。
        /// @param object 所属させる所有参照。
        void Attach(const std::shared_ptr<GameObject>& object);
        /// @brief 既存個体への参照を解決してから初期化する。
        /// @param object 所属済みの個体。
        void Initialize(GameObject& object);
        // このシーン専用のコライダー管理(objectsより先に宣言=objectsの後に破棄される)
        col2d::ColliderManager m_colMgr;
        std::unordered_map<uint32_t, std::vector<std::shared_ptr<GameObject>>> m_objects;
        std::vector<std::shared_ptr<GameObject>> m_pendingObjects;    //一時保存オブジェクト配列
        bool m_isUpdating;                              //更新状況
    };
}
