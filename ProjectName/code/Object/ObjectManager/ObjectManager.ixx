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

        /// <summary>
        /// オブジェクト追加
        /// </summary>
        /// <param name="object">追加するオブジェクト</param>
        template<typename... Args>
        inline void AddObject(GameObject* object, Args&&... args)
        {
            // 更新中なら一時保存、そうでないなら新規追加
            if (isUpdate)
            {
                object->SetColliderManager(&m_colMgr);
                object->OnAttach();
                pendingObjects.emplace_back(object);
                return;
            }

            // 新規追加
            object->SetColliderManager(&m_colMgr);
            object->OnAttach();
            auto obj = std::shared_ptr<GameObject>(object, std::forward<Args>(args)...);
            objects[object->MyObjectTag()].emplace_back(obj);
        }

        /// <summary>更新走査の外で新しい個体だけを参照解決・初期化し、所有する。失敗時は個体を破棄する。</summary>
        /// <param name="object">未登録・未初期化の個体。所有権を渡す。</param>
        void AddInitializedObject(std::unique_ptr<GameObject> object);

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
        // このシーン専用のコライダー管理(objectsより先に宣言=objectsの後に破棄される)
        col2d::ColliderManager m_colMgr;
        std::unordered_map<uint32_t, std::vector<std::shared_ptr<GameObject>>> objects;
        std::vector<GameObject*> pendingObjects;    //一時保存オブジェクト配列
        bool isUpdate;                              //更新状況
    };
}
