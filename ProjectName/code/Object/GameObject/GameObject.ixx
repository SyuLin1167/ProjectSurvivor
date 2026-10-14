export module Object.GameObject;

import <memory>;
import <typeindex>;
import <functional>;
import <unordered_map>;

export import MyLib.Math.PropVector2;
export import ObjectState;
export import ObjectTag;
export import AppContext;
export import Component.ComponentManager;
export import Collider.ColliderManager;

using namespace math;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    /// <summary>
    /// ゲーム内オブジェクトの基底クラス
    /// </summary>
    export class GameObject
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        GameObject();

        /// <summary>
        /// デストラクタ
        /// </summary>
        virtual ~GameObject();

        /// <summary>
        ///  参照するオブジェクト取得
        /// </summary>
        virtual void GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj) {};

        /// <summary>
        /// 初期化処理
        /// </summary>
        virtual void Init() = 0;

        /// <summary>
        /// 更新
        /// </summary>
        virtual void Update() {};

        /// <summary>
        /// 後更新
        /// </summary>
        virtual void LateUpdate() {};

        /// <summary>
        /// コンポーネント郡更新
        /// </summary>
        void UpdateComponents();

        /// <summary>
        /// コンポーネント郡描画
        /// </summary>
        void DrawComponents();

        /// <summary>
        /// 描画
        /// </summary>
        virtual void Draw() = 0;

        /// <summary>
        /// 現在の状態
        /// </summary>
        /// <returns>状態</returns>
        uint32_t NowState() const
        {
            return m_state;
        };

        /// <summary>
        /// 座標(アクセッサー)
        /// </summary>
        /// <returns>座標(外部用)</returns>
        PropVector2<float> AccessPos()
        {
            return PropVector2<float>(m_pos);
        }

        /// <summary>
        /// 速度(アクセッサー)
        /// </summary>
        /// <returns>速度(外部用)</returns>
        PropVector2<float> AccessVel()
        {
            return PropVector2<float>(m_velocity);
        }

        /// <summary>
        /// 自身のタグを取得
        /// </summary>
        /// <returns>自身のタグ</returns>
        virtual ObjectTag MyObjectTag() const = 0;

        /// <summary>
        /// 所属シーンのコライダー管理を設定(ObjectManagerが呼ぶ)
        /// </summary>
        void SetColliderManager(col2d::ColliderManager* mgr)
        {
            m_colMgr = mgr;
        }

        /// <summary>
        /// シーンへ所属した直後に呼ばれる(ここからColMgr()が使用可能)
        /// </summary>
        virtual void OnAttach() {}

    protected:
        /// <summary>
        /// 所属シーンのコライダー管理(未所属ならnullptr)
        /// </summary>
        col2d::ColliderManager* ColMgrPtr() const
        {
            return m_colMgr;
        }

        /// <summary>
        /// 所属シーンのコライダー管理(Init以降で使用)
        /// </summary>
        col2d::ColliderManager& ColMgr() const
        {
            return *m_colMgr;
        }

        col2d::ColliderManager* m_colMgr = nullptr;   //所属シーンのコライダー管理
        std::shared_ptr<component::ComponentManager> m_compMgr;
        const float& m_deltaTime;             //デルタタイム
        Vector2f m_pos;           //座標
        Vector2f m_velocity;      //速さ
        uint32_t m_state;         //状態
    };
}