export module Collider.TileCollider;
import <future>;
import <string>;
import <queue>;
import <unordered_map>;
import MyLib.Shape.Rect;
import MyLib.Loading;
export import Collider;
export import Object.MapInfo;
import Collider.RectCollider;

using namespace math;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    // 前方宣言
    class TileColliderVisitor;

    /// <summary>
    /// タイルの隣接フラグ
    /// </summary>
    export enum TileFlag : uint8_t
    {
        NONE = 0,            // なし
        LEFT = 1u << 1,      // 左側
        RIGHT = 1u << 2,     // 右側
        TOP = 1u << 3,       // 上側
        BOTTOM = 1u << 4,    // 下側
        ALL = LEFT | RIGHT | TOP | BOTTOM  // 全方向
    };

    /// <summary>
    /// タイルの種類
    /// </summary>
    export enum class TileType : uint8_t
    {
        SOLID = 1, // 通常ブロック
        ONE_WAY_TOP = 2  // 上からのみ当たり（足場）
    };

    /// <summary>
    /// タイル情報
    /// </summary>
    struct TileInfo
    {
        TileType type;                          // タイルの種類
        uint8_t adjacentFlag;                   // 隣接するタイルのフラグ
        std::unique_ptr<RectCollider> collider; // タイルに対応する矩形コライダー
    };

    /// <summary>
    /// タイルコライダー
    /// </summary>
    /// <remarks>
    /// タイルマップのタイルに対するコライダー
    /// </remarks>
    export class TileCollider final : public Collider
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="def">コライダー定義</param>
        /// <param name="info">マップ情報</param>
        /// <param name="fileName">ファイル名</param>
        TileCollider(ColliderDef* def, const object::MapInfo& info, std::string fileName);
        
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="def">コライダー定義</param>
        /// <param name="info">マップ情報</param>
        /// <param name="fileName">ファイル名</param>
        TileCollider(ColliderDef* def, std::shared_future<object::MapInfo> info, std::string fileName);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~TileCollider() override;

        /// <summary>
        /// カテゴリーの生成
        /// </summary>
        /// <param name="ownerID">所有者のID</param>
        void GenerateCategory(uint32_t ownerID = 0) override
        {
            m_filter.category = MakeKey(ShapeType::TILE, ownerID);
        }

        /// <summary>
        /// タイルコライダーの衝突判定
        /// </summary>
        /// <param name="rect">矩形</param>
        /// <returns>衝突しているか</returns>
        bool IsColliding(const shape::Rect& rect);

        /// <summary>
        /// ヒットしたタイルのキーを取得
        /// </summary>
        /// <returns>ヒットしたタイルのキー/returns>
        std::queue<std::pair<size_t, size_t>> TakeHitTileKeys()
        {
            return std::exchange(m_hitTileKeys, {});
        }

        /// <summary>
        /// タイル情報を取得
        /// </summary>
        /// <param name="key">タイルのキー</param>
        /// <param name="index">タイルのインデックス</param>
        /// <returns>タイル情報へのポインタ</returns>
        inline const TileInfo* GetTileInfo(size_t key, size_t index) const
        {
            if (auto it = m_tileColliders.find(key); it != m_tileColliders.end())
            {
                if (index < it->second.size())
                {
                    return &it->second[index];
                }
            }
            return nullptr;
        }

        /// <summary>
        /// タイルサイズを取得
        /// </summary>
        /// <returns>タイルサイズ</returns>
        const Vector2f& GetTileSize() const
        {
            return m_mapInfo.tileSize;
        }

    private:
        /// <summary>
        /// タイルのキーを生成
        /// </summary>
        /// <param name="cx">チャンクのX座標</param>
        /// <param name="cy">チャンクのY座標</param>
        /// <returns>生成されたキー</returns>
        static inline uint64_t MakeTileKey(uint32_t cx, uint32_t cy)
        {
            return (static_cast<uint64_t>(cy) << 32) | static_cast<uint64_t>(cx);
        }

        /// <summary>
        /// 値を指定の範囲で剰余演算
        /// </summary>
        /// <param name="value">値</param>
        /// <param name="mod">剰余の値</param>
        /// <returns>剰余演算の結果</returns>
        static inline size_t Mod(size_t value, size_t mod)
        {
            return value % mod;
        }

        /// <summary>
        /// 隣接するタイルのフラグを取得
        /// <summary>
        /// <param name="tileLayer">タイルレイヤー</param>
        /// <param name="x">タイルのX座標</param>
        /// <param name="y">タイルのY座標</param>
        uint8_t AdjacentTileAt(const std::vector<size_t>& mapData, size_t x, size_t y) const;

        /// <summary>
        /// コライダーを訪問
        /// </summary>
        /// <param name="visitor">訪問するビジター</param>
        void Accept(ColliderVisitor& visitor) override {}

        /// <summary>
        /// タイルコライダーの生成
        /// </summary>
        /// <param name="mapData">マップデータ</param>
        void BuildTileColliders(std::vector<size_t> mapData);

        /// <summary>
        /// タイルコライダーを構築して返す
        /// </summary>
        /// <param name="mapInfo">マップ情報</param>
        /// <param name="mapData">マップデータ</param>
        std::unordered_map<size_t, std::vector<TileInfo>> BuildTileCollidersFor(const object::MapInfo& mapInfo, const std::vector<size_t>& mapData) const;

        /// <summary>
        /// 構築結果の反映
        /// </summary>
        void Commit(task::Loading* loading, const object::MapInfo& mapInfo, std::shared_ptr<std::unordered_map<size_t, std::vector<TileInfo>>> built);

        object::MapInfo m_mapInfo;      // マップ情報

        std::unordered_map<size_t, std::vector<TileInfo>> m_tileColliders;  // タイル状のコライダーを保持するベクター
        std::queue<std::pair<size_t, size_t>> m_hitTileKeys;                // ヒットしたタイルのキー
    };
}

