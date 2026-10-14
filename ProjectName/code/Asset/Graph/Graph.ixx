export module Asset.Graph;
import <unordered_map>;
import <string>;
import Asset.AssetBase;

/// <summary>
/// アセット関連
/// </summary>
export namespace asset
{
    /// <summary>
    /// 画像の管理を担当
    /// </summary>
    export class Graph final :public AssetBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Graph();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Graph();

        /// <summary>
        /// ハンドル生成
        /// </summary>
        /// <param name="handleName">ハンドル名</param>
        /// <param name="graphName">画像名</param>
        void CreateHandle(std::string handleName, std::string graphName) override;

        /// <summary>
        /// ハンドル生成(非同期)
        /// </summary>
        /// <param name="handleName">ハンドル名</param>
        /// <param name="graphName">画像名</param>
        void CreateHandleAsync(std::string handleName, std::string graphName) override;

        /// <summary>
        /// ハンドル削除
        /// </summary>
        /// <param name="name">削除するハンドル名</param>
        void DeleteHandle(std::string name) override;

        /// <summary>
        /// ハンドル取得
        /// </summary>
        /// <param name="name">取得するハンドル名</param>
        /// <returns>ハンドル</returns>
        int GetHandle(const std::string& name) const
        {
            if (auto it = handles.find(name); it != handles.end())
            {
                return it->second;
            }
            return -1;
        }
    private:
        std::unordered_map<std::string, int> handles;   //ハンドル群
    };
}

