export module Asset.Animation;

import <memory>;
import <queue>;
import <unordered_map>;
import <string>;

import Asset.AssetBase;
import Asset.DivisionGraph;
export import Asset.AnimationDef;
import MyLib.Math.Vector2;

/// <summary>
/// アセット関連
/// </summary>
export namespace asset
{
    /// <summary>
    /// アニメーションを担当するクラス
    /// </summary>
    export class Animation final :public AssetBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Animation();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Animation();

        /// <summary>
        /// ハンドル生成
        /// </summary>
        /// <param name="handleName">ハンドル名</param>
        /// <param name="graphName">画像名</param>
        void CreateHandle(std::string handleName, std::string graphName) override;

        /// <summary>
        /// ハンドル生成
        /// </summary>
        /// <param name="handleName">ハンドル名</param>
        /// <param name="graphName">画像名</param>
        /// <param name="sizeX">分割サイズX</param>
        /// <param name="sizeY">分割サイズY</param>
        void CreateHandle(std::string handleName, std::string graphName, const int& sizeX, const int& sizeY);

        /// <summary>
        /// ハンドル生成(非同期)
        /// </summary>
        /// <param name="handleName">ハンドル名</param>
        /// <param name="graphName">画像名</param>
        void CreateHandleAsync(std::string handleName, std::string graphName) override;

        /// <summary>
        /// ハンドル生成(非同期)
        /// </summary>
        /// <param name="handleName">ハンドル名</param>
        /// <param name="graphName">画像名</param>
        /// <param name="sizeX">分割サイズX</param>
        /// <param name="sizeY">分割サイズY</param>
        void CreateHandleAsync(std::string handleName, std::string graphName ,const int& sizeX, const int& sizeY);

        /// <summary>
        /// アニメーション作成
        /// </summary>
        /// <param name="animName">アニメーション名</param>
        /// <param name="animDef">アニメーション定義</param>
        void CreateAnim(const AnimationDef& animDef);

        /// <summary>
        /// アニメーション群作成
        /// </summary>
        /// <param name="animName">カテゴリー名</param>
        /// <param name="animDef">アニメーション定義群</param>
        void CreateAnimCategory(const std::string& category, const std::vector<AnimationDef> animDefs);

        /// <summary>
        /// ハンドル削除
        /// </summary>
        /// <param name="name">削除するハンドル名</param>
        void DeleteHandle(std::string name) override;

        /// <summary>
        /// アニメーション削除
        /// </summary>
        /// <param name="animName">削除するアニメーション</param>
        void DeleteAnim(const std::string& animName);

        /// <summary>
        /// アニメーション取得
        /// </summary>
        /// <param name="animName">取得するアニメーション</param>
        /// <returns>取得対象のアニメーション</returns>
        const std::pair<int*, AnimationDef> GetAnim(const std::string& animName) const
        {
            if (m_animations.find(animName) != m_animations.end())
            {
                return m_animations.at(animName);
            }
            return {};
        }

    private:
        std::unique_ptr<DivisionGraph> m_divGraph; // 分割画像の管理を担当するクラス
        std::unordered_map<std::string, std::pair<int*, AnimationDef>> m_animations;  // アニメーション群
    };
}