export module Component.ComponentManager;

import <unordered_map>;
import <memory>;
import <typeindex>;

import Component;

/// <summary>
/// コンポーネント関連
/// </summary>
export namespace component
{
    /// <summary>
    /// コンポーネントを管理する
    /// </summary>
    export class ComponentManager final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        ComponentManager() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~ComponentManager() = default;

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <param name="deltaTime">デルタタイム</param>
        void Update(const float& deltaTime);

        /// <summary>
        /// 描画処理
        /// </summary>
        virtual void Draw();

        /// <summary>
        /// コンポーネント追加
        /// </summary>
        /// <typeparam name="T">コンポーネント名</typeparam>
        /// <typeparam name="...Args">引数群</typeparam>
        /// <param name="...args">引数群</param>
        /// <returns>追加したコンポーネント</returns>
        template<typename T, typename... Args>
        std::shared_ptr<T> AddComponent(Args&&... args)
        {
            auto component = std::make_shared<T>(std::forward<Args>(args)...);
            m_components[typeid(T)] = component;
            return component;
        }

        /// <summary>
        /// コンポーネントの取得
        /// </summary>
        /// <typeparam name="T">取得対象のコンポーネント名</typeparam>
        /// <returns>コンポーネント</returns>
        template<typename T>
        std::shared_ptr<T> GetComponent()
        {
            if (HasComponent<T>())
            {
                return std::dynamic_pointer_cast<T>(m_components[typeid(T)]);
            }
            return nullptr;
        }

        /// <summary>
        /// コンポーネントの存在確認
        /// </summary>
        /// <typeparam name="T">確認対象のコンポーネント</typeparam>
        /// <returns>存在するかどうか</returns>
        template<typename T>
        bool HasComponent()
        {
            return m_components.find(typeid(T)) != m_components.end();
        }

        /// <summary>
        /// コンポーネント削除
        /// </summary>
        /// <param name="component">削除するコンポーネント</param>
        template<typename T>
        void RemoveComponent()
        {
            m_components.erase(typeid(T));
        }

    private:
        std::unordered_map<std::type_index, std::shared_ptr<component::ComponentBase>> m_components;   //コンポーネント群
    };
}
