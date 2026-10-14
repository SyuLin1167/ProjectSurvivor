export module Asset.AssetManager;

import <memory>;
import <string>;
import <typeindex>;
import <unordered_map>;

import Asset.AssetBase;

/// <summary>
/// アセット関連
/// </summary>
export namespace asset
{
    /// <summary>
    /// アセット全般を管理する
    /// </summary>
    export class AssetManager final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        AssetManager() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~AssetManager()
        {
            m_registry.clear();
        }

        /// <summary>
        /// アセット読み込み
        /// </summary>
        /// <typeparam name="T">アセットの種類</typeparam>
        /// <typeparam name="...Args">引数</typeparam>
        /// <param name="name">アセット名</param>
        /// <param name="path">ファイルパス</param>
        template<typename T, typename... Args>
        void Load(std::string name, std::string path)
        {
            Fetch<T>()->CreateHandle(name, path);
        }

        /// <summary>
        /// アセット読み込み(非同期
        /// </summary>
        /// <typeparam name="T">アセットの種類</typeparam>
        /// <typeparam name="...Args">引数</typeparam>
        /// <param name="name">アセット名</param>
        /// <param name="path">ファイルパス</param>
        template<typename T, typename... Args>
        void LoadAsync(std::string name, std::string path)
        {
            Fetch<T>()->CreateHandleAsync(name, path);
        }

        /// <summary>
        /// フェッチ
        /// </summary>
        /// <typeparam name="T">アセットの種類</typeparam>
        /// <returns>アセット</returns>
        template<typename T>
        T* Fetch()
        {
            // 存在する場合はそれを返す
            auto it = m_registry.find(typeid(T));
            if (it != m_registry.end())
            {
                return static_cast<T*>(it->second.get());
            }

            // 存在しない場合は新規作成して返す
            m_registry[typeid(T)] = std::make_unique<T>();
            return static_cast<T*>(m_registry.at(typeid(T)).get());
        }

        /// <summary>
        /// アセット削除
        /// </summary>
        /// <typeparam name="T">アセットの種類</typeparam>
        /// <param name="name">アセット名</param>
        template<typename T>
        void DeleteHandle(std::string name)
        {
            m_registry.at(typeid(T))->DeleteHandle(name);
        }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<AssetBase>> m_registry;   //アセット登録群
    };
}
