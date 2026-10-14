module;
#include <ranges>

export module ColliderFilter;
import <vector>;
import <cstdint>;
import <algorithm>;

/// <summary>
/// 当たり判定関連
/// </summary>
export namespace col2d
{
    /// <summary>
    /// 形状の種類
    /// </summary>
    export enum ShapeType : uint32_t
    {
        RECT = 1u << 0,   //矩形
        CIRCLE = 1u << 1,    // 円
        POLY = 1u << 2,   // 多角形
        TILE = 1u << 3,   // タイル
    };

    /// <summary>
    /// 形状の上位ビットを取得
    /// </summary>
    /// <param name="_shape">形状の種類</param>
    /// <returns>上位ビット</returns>
    export inline uint64_t ShapeHi(uint32_t shape)
    {
        return static_cast<uint64_t>(shape) << 32;
    }

    /// <summary>
    /// キーを生成
    /// </summary>
    /// <param name="shape">形状の種類</param>
    /// <param name="ownerID">所有者のID</param>
    /// <returns>生成されたキー</returns>
    export inline uint64_t MakeKey(uint32_t shape, uint32_t ownerID)
    {
        return ShapeHi(shape) | static_cast<uint64_t>(ownerID);
    }

    /// <summary>
    /// 最小の符号付きビットを取得
    /// </summary>
    /// <param name="value">値</param>
    /// <returns>最小の符号付きビット</returns>
    export inline uint32_t LeastSignBit(const uint32_t& value)
    {
        return value & (~value + 1);
    }

    /// <summary>
    /// フィルタ
    /// </summary>
    export struct Filter final
    {
        uint64_t category; // カテゴリー
        std::vector<uint64_t> masks; // マスク

        /// <summary>
        /// コンストラクタ
        /// </summary>
        Filter() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Filter() = default;

        /// <summary>
        /// マスクを追加
        /// </summary>
        /// <param name="shape">追加する形状</param>
        /// <param name="ownerID">追加する所有者ID</param>
        void AddMask(uint32_t shape, uint32_t ownerID)
        {
            while (shape)
            {
                uint32_t lsb = LeastSignBit(shape);


                // 既に同じマスクが存在するか確認
                if (std::ranges::find(masks, MakeKey(lsb, ownerID)) != masks.end())
                {
                    shape &= ~lsb;
                    continue;
                }


                // 既に同じ形状のマスクが存在する場合は、所有者IDを追加
                if (auto it = std::ranges::find_if(masks, [&lsb](uint64_t mask) { return (mask >> 32) == lsb; });
                    it != masks.end())
                {
                    *it |= ownerID;
                    shape &= ~lsb;
                    continue;
                }

                // 新しいマスクを追加
                masks.push_back(MakeKey(lsb, ownerID));
                shape &= ~lsb;
            }
        }

        /// <summary>
        /// マスクを削除
        /// </summary>
        /// <param name="_mask">削除するマスク</param>
        void RemoveMask(const uint64_t& mask)
        {
            if (auto it = std::find(masks.begin(), masks.end(), mask); it != masks.end())
            {
                masks.erase(it);
            }
        }

        /// <summary>
        /// マスクをクリア
        /// </summary>
        void ClearMasks()
        {
            masks.clear();
        }

        /// <summary>
        /// マスクが含まれているか確認
        /// </summary>
        /// <param name="_mask">確認するマスク</param>
        /// <returns>含まれているかどうか</returns>
        bool HasMask(const uint64_t& mask) const
        {
            return std::find(masks.begin(), masks.end(), mask) != masks.end();
        }
    };
}