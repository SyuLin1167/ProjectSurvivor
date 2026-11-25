module;
#include <json.hpp>

export module Object.MapInfo;
import MyLib.File.JsonValue;

import MyLib.Math.Vector2;

using json = nlohmann::json;
using namespace math;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    /// <summary>
    /// マップ情報
    /// </summary>
    export struct MapInfo
    {
        Vector2<size_t> mapSize;    // マップサイズ(タイル数)
        Vector2f tileSize; // タイルサイズ(ピクセル数)
        Vector2u chunkSize;               // チャンクサイズ

        /// <summary>
        /// JSONからマップ情報を生成
        /// </summary>
        /// <param name="j">JSONデータ</param>
        /// <returns>マップ情報</returns>
        MapInfo FromJson(const json& j)
        {
            mapSize.x = file::jsonValue::Read<size_t>(j, "width");
            mapSize.y = file::jsonValue::Read<size_t>(j, "height");
            tileSize.x = file::jsonValue::Read<float>(j, "tilewidth");
            tileSize.y = file::jsonValue::Read<float>(j, "tileheight");
            return *this;
        }
    };
}