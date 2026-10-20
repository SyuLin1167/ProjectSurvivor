export module Object.Map;

import Phase1.Config;

import Object.GameObject;
import Object.MapInfo;
import Asset.AssetManager;
import MyLib.Math.Vector2;

export import <vector>;
import <future>;

using json = nlohmann::json;
using namespace math;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    /// <summary>
    /// タイルマップに関する処理を行う
    /// </summary>
    export class Map final :public GameObject
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        explicit Map(bool prototype = false);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Map();

        /// <summary>
        /// 初期化処理
        /// </summary>
        void Init() override;

        /// <summary>
        /// シーン所属時の処理(タイルコライダー生成)
        /// </summary>
        void OnAttach() override;

        /// <summary>
        /// 描画
        /// </summary>
        void Draw() override;

        /// <summary>
        /// 描画範囲算出
        /// </summary>
        /// <param name="pos">座標</param>
        /// <param name="range">範囲</param>
        /// <param name="length">限界範囲</param>
        void CalcDrawRange(int pos, std::pair < size_t, size_t>& range, size_t length);

        /// <summary>
        /// 自身のタグ
        /// </summary>
        /// <returns>Map</returns>
        ObjectTag MyObjectTag() const override
        {
            return ObjectTag::MAP;
        }

    private:
        bool m_prototype; // Phase 1試作経路の選択
        phase1::Config m_phase1Config{}; // JSONから読み込んだ試作設定
        MapInfo m_mapInfo;                  // マップ情報
        std::unordered_map<size_t, std::vector<size_t>> m_mapData;   //マップデータ

        std::pair<size_t, size_t> rangeX;           //描画範囲X
        std::pair<size_t, size_t> rangeY;           //描画範囲Y

        col2d::ColliderID colID;            // コライダーID
        std::shared_future<MapInfo> m_infoFuture;   // MapInfo通知用future
    };
}
