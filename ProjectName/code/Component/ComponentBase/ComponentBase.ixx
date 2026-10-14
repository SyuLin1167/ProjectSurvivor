export module Component;

// 前方宣言
export namespace object
{
    class GameObject;
}

/// <summary>
/// コンポーネント関連
/// </summary>
export namespace component
{
    /// <summary>
    /// コンポーネントの基底クラス
    /// </summary>
    export class ComponentBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        ComponentBase() = default;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="owner">所有者</param>
        /// <param name="order">更新順番</param>
        explicit ComponentBase(class object::GameObject* owner);

        /// <summary>
        /// デストラクタ
        /// </summary>
        virtual ~ComponentBase() = default;

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <param name="deltaTime">デルタタイム</param>
        virtual void Update(const float& deltaTime) {};

        /// <summary>
        /// 描画処理
        /// </summary>
        virtual void Draw() {};

    protected:
        class object::GameObject* m_owner;  //所有者
    };
}
