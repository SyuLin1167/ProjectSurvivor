module;
#include <json.hpp>
#include <thread>
#include <fstream>

module Collider.TileCollider;
import <algorithm>;
import MyLib.File.MemMapFile;
import MyLib.Loading.LoadingContext;
import MyLib.TileChunkUtil;
import MyLib.Math.Vector2;
import AppContext;
import GameSystem.Window;
import Collider.TileColliderVisitor;

using json = nlohmann::json;
using namespace math;
using namespace gameSystem;

namespace col2d
{
    TileCollider::TileCollider(ColliderDef* def, const object::MapInfo& info, std::string fileName)
        : Collider(def)
    {
        // ビジターの初期化
        m_visitor = std::make_unique<TileColliderVisitor>(*this);

        // マップ構造の読み込み
        auto path = AppCtx::FileSystem().Resolve("data://" + fileName);
        auto mapDataSf = AppCtx::FileSystem().csvIO.CreateArrayAsync<size_t>(path).share();

        // タイルコライダー生成タスク
        auto task = [this, loading = task::LoadingContext::Get(), mapDataSf, info]()
        {
            const auto& mapData = mapDataSf.get();
            auto built = std::make_shared<std::unordered_map<size_t, std::vector<TileInfo>>>(BuildTileCollidersFor(info, mapData));
            Commit(loading, info, std::move(built));
        };

        // タスクの登録または即時実行
        if (task::LoadingContext::Get())
        {
            task::LoadingContext::Get()->AddTask(task::INIT, task);
        }
        else
        {
            task();
        }
    }

    TileCollider::TileCollider(ColliderDef* def, std::shared_future<object::MapInfo> info, std::string fileName)
        : Collider(def)
    {
        // ビジターの初期化
        m_visitor = std::make_unique<TileColliderVisitor>(*this);

        // マップ構造の読み込み
        auto path = AppCtx::FileSystem().Resolve("data://" + fileName);
        auto mapDataSf = AppCtx::FileSystem().csvIO.CreateArrayAsync<size_t>(path).share();
 
        // タイルコライダー生成タスク
        auto task = [this, loading = task::LoadingContext::Get(), mapDataSf, info]()
        {
            const object::MapInfo mapInfo = info.get();
            const auto& mapData = mapDataSf.get();
            auto built = std::make_shared<std::unordered_map<size_t, std::vector<TileInfo>>>(BuildTileCollidersFor(mapInfo, mapData));
            Commit(loading, mapInfo, std::move(built));
        };
        // タスクの登録または即時実行
        if (task::LoadingContext::Get())
        {
            task::LoadingContext::Get()->AddTask(task::INIT, task);
        }
        else
        {
            // エラーを通知
            throw std::runtime_error("TileCollider must be created within a LoadingContext.");
        }
    }

    TileCollider::~TileCollider() = default;

    void TileCollider::Commit(task::Loading* loading, const object::MapInfo& mapInfo, std::shared_ptr<std::unordered_map<size_t, std::vector<TileInfo>>> built)
    {
        // メンバーへの反映はメインスレッドで行う
        auto apply = [this, mapInfo, built]()
        {
            m_mapInfo = mapInfo;
            m_tileColliders = std::move(*built);
        };
        if (loading)
        {
            loading->PostToMain(std::move(apply));
        }
        else
        {
            apply();
        }
    }

    bool TileCollider::IsColliding(const shape::Rect& rect)
    {
        while(!m_hitTileKeys.empty())
        {
            m_hitTileKeys.pop();
        }

        // タイル範囲を計算(0～タイル幅)
        Vector2f tileLeft = rect.pos / m_mapInfo.tileSize;
        tileLeft.x = std::floor(tileLeft.x);
        tileLeft.y = std::floor(tileLeft.y);
        Vector2f tileRight = (rect.pos + rect.size) / m_mapInfo.tileSize;
        tileRight.x = std::ceil(tileRight.x) - 1;
        tileRight.y = std::ceil(tileRight.y) - 1;

        // タイル範囲が無効な場合はfalseを返す
        if (tileLeft.x > tileRight.x || tileLeft.y > tileRight.y)
        {
            return false;
        }
        if (tileRight.x < 0 || tileRight.y < 0 ||
            tileLeft.x >= m_mapInfo.mapSize.x ||
            tileLeft.y >= m_mapInfo.mapSize.y)
        {
            return false;
        }

        // チャンクサイズを取得
        auto chunkSize = m_mapInfo.chunkSize;

        // チャンク範囲を計算
        Vector2u chunkLeft{};
        chunkLeft.x = static_cast<uint32_t>(std::floor(tileLeft.x / chunkSize.x));
        chunkLeft.y = static_cast<uint32_t>(std::floor(tileLeft.y / chunkSize.y));
        Vector2u chunkRight{};
        chunkRight.x = static_cast<uint32_t>(std::floor(tileRight.x / chunkSize.x));
        chunkRight.y = static_cast<uint32_t>(std::floor(tileRight.y / chunkSize.y));

        // チャンク内のタイルコライダーを走査
        for (uint32_t cy = chunkLeft.y; cy <= chunkRight.y; ++cy)
        {
            for (uint32_t cx = chunkLeft.x; cx <= chunkRight.x; ++cx)
            {
                // チャンクのキーを生成
                size_t key = MakeTileKey(cx, cy);

                // チャンクのタイルコライダーを取得
                auto iter = m_tileColliders.find(key);
                if (iter == m_tileColliders.end())
                {
                    continue;
                }

                // チャンク内のタイル範囲を計算
                Vector2<size_t> localLeft{};
                localLeft.x = (cx == chunkLeft.x) ? Mod(static_cast<size_t>(tileLeft.x), chunkSize.x) : 0;
                localLeft.y = (cy == chunkLeft.y) ? Mod(static_cast<size_t>(tileLeft.y), chunkSize.y) : 0;
                Vector2<size_t> localRight{};
                localRight.x = (cx == chunkRight.x) ? Mod(static_cast<size_t>(tileRight.x), chunkSize.x) : (chunkSize.x - 1);
                localRight.y = (cy == chunkRight.y) ? Mod(static_cast<size_t>(tileRight.y), chunkSize.y) : (chunkSize.y - 1);

                // チャンク内のタイル範囲を走査
                for (size_t ty = localLeft.y; ty <= localRight.y; ++ty)
                {
                    for (size_t tx = localLeft.x; tx <= localRight.x; ++tx)
                    {
                        // タイルのインデックスを生成
                        size_t index = ty * chunkSize.x + tx;

                        // タイルコライダーが存在するか確認
                        if (index >= iter->second.size() || !iter->second[index].collider)
                        {
                            continue;
                        }
                        
                        if (iter->second[index].collider->GetRect().AABB(rect))
                        {
                            m_hitTileKeys.push({ key, index });
                        }
                    }
                }
            }
        }

        // 範囲内にタイルコライダーが存在する場合はtrueを返す
        if (!m_hitTileKeys.empty())
        {
            return true;
        }

        // 範囲内にタイルコライダーが存在しない場合はfalseを返す
        return false;
    }

    uint8_t TileCollider::AdjacentTileAt(const std::vector<size_t>& _mapData, size_t _x, size_t _y) const
    {
        uint8_t flag = 0;
        const size_t tileIndex = _y * m_mapInfo.mapSize.x + _x;

        // 左のタイルが存在するか
        if (_x > 0 && _mapData[tileIndex - 1] != 0)
        {
            flag |= TileFlag::LEFT;
        }
        // 右のタイルが存在するか
        if (_x < m_mapInfo.mapSize.x - 1 && _mapData[tileIndex + 1] != 0)
        {
            flag |= TileFlag::RIGHT;
        }
        // 上のタイルが存在するか
        if (_y > 0 && _mapData[tileIndex - m_mapInfo.mapSize.x] != 0)
        {
            flag |= TileFlag::TOP;
        }
        // 下のタイルが存在するか
        if (_y < m_mapInfo.mapSize.y - 1 && _mapData[tileIndex + m_mapInfo.mapSize.x] != 0)
        {
            flag |= TileFlag::BOTTOM;
        }
        return flag;
    }

    void TileCollider::BuildTileColliders(std::vector<size_t> mapData)
    {
        m_tileColliders = BuildTileCollidersFor(m_mapInfo, mapData);
    }

    std::unordered_map<size_t, std::vector<TileInfo>> TileCollider::BuildTileCollidersFor(const object::MapInfo& mapInfo, const std::vector<size_t>& mapData) const
    {
        // 隣接フラグ算出用の一時TileCollider無しで済むよう、ローカルのラムダで算出
        auto adjacentAt = [&](size_t x, size_t y) -> uint8_t
        {
            uint8_t flag = 0;
            const size_t tileIndex = y * mapInfo.mapSize.x + x;
            if (x > 0 && mapData[tileIndex - 1] != 0) { flag |= TileFlag::LEFT; }
            if (x < mapInfo.mapSize.x - 1 && mapData[tileIndex + 1] != 0) { flag |= TileFlag::RIGHT; }
            if (y > 0 && mapData[tileIndex - mapInfo.mapSize.x] != 0) { flag |= TileFlag::TOP; }
            if (y < mapInfo.mapSize.y - 1 && mapData[tileIndex + mapInfo.mapSize.x] != 0) { flag |= TileFlag::BOTTOM; }
            return flag;
        };

        // チャンク化してタイルコライダー生成
        return tile::BuildChunkedGrid<TileInfo>(
            mapInfo.mapSize.x,
            mapInfo.mapSize.y,
            mapInfo.chunkSize,
            [&](size_t x, size_t y, size_t gidx, size_t , size_t) -> std::optional<TileInfo>
            {
                if (mapData[gidx] == 0)
                {
                    return std::nullopt;
                }

                auto adjacentFlag = adjacentAt(x, y);
                if (adjacentFlag == TileFlag::ALL)
                {
                    return std::nullopt;
                }

                TileInfo tileInfo{};
                tileInfo.adjacentFlag = adjacentFlag;
                tileInfo.type = static_cast<TileType>(mapData[gidx]);

                ColliderDef colDef{};
                colDef.localPos = Vector2f(x * mapInfo.tileSize.x, y * mapInfo.tileSize.y);
                colDef.isActive = true;

                tileInfo.collider = std::make_unique<RectCollider>(&colDef, mapInfo.tileSize);

                return tileInfo;
            });
    }
}
