module;
#include <DxLib.h>
#include <json.hpp>
#include <fstream>
#include <functional>
#include <algorithm>

module Object.Map;

import Phase1.Config;

import <future>;

import MyLib.Loading.LoadingContext;
import MyLib.Math.Vector2;
import MyLib.TileChunkUtil;
import GameSystem.Camera;
import Asset.DivisionGraph;
import GameSystem.Window;

using namespace gameSystem;
using namespace math;

namespace object
{
    Map::Map(bool prototype) : m_prototype(prototype)
    {
        if (m_prototype)
        {
            // 既存JsonIOで試作値を読み込む。欠損・不正な設定は例外として報告する。
            m_phase1Config = phase1::Config::FromJson(AppCtx::FileSystem().jsonIO.Load(
                AppCtx::FileSystem().GetDataDir() / "Phase1Config"));
            // 空き地にはタイル画像・TileColliderが不要。ワールド寸法はInitで直接設定する。
            return;
        }
        // マップ構成読み込み
        auto path = AppCtx::FileSystem().Resolve("data://MapTip.csv");
        auto mapDataArray = AppCtx::FileSystem().csvIO.CreateArray<size_t>(path);

        // タイル情報読み込み
        auto mapInfo = AppCtx::FileSystem().jsonIO.Load(AppCtx::FileSystem().Resolve("data://MapData")); 

        // 最終的な MapInfo を届けるための promise/future
        auto infoPromise = std::make_shared<std::promise<object::MapInfo>>();

        // マップ生成
        task::LoadingContext::Get()->AddTask(task::INIT, [this, loading = task::LoadingContext::Get(), mapDataArray, mapInfo, infoPromise]() {
            try
            {
                // ワーカー内では一時変数にのみ書き込む
                MapInfo info;
                info.FromJson(mapInfo);

                // チャンクサイズを取得
                info.chunkSize = Window::GetWindowData()->SIZE / info.tileSize.Half();

                // 待機中のTileColliderへ通知
                infoPromise->set_value(info);

                //チャンク化してマップデータ格納
                auto chunked = tile::BuildChunkedGrid<size_t>(
                    info.mapSize.x,
                    info.mapSize.y,
                    info.chunkSize,
                    [&](size_t, size_t, size_t gidx, size_t, size_t)->std::optional<size_t>
                    {
                        return std::optional<size_t>{ mapDataArray[gidx] };
                    });

                // メンバーへの反映はメインスレッドで行う
                loading->PostToMain([this, info, chunked = std::move(chunked)]() mutable {
                    m_mapInfo = info;
                    m_mapData = std::move(chunked);
                    });
            }
            catch (...)
            {
                // 待機側が無限待機しないよう例外を伝える(set_value済みなら無視)
                try { infoPromise->set_exception(std::current_exception()); }
                catch (const std::future_error&) {}
                throw;
            }
            });

        // 当たり判定はシーン所属後(Init)に生成する
        m_infoFuture = infoPromise->get_future().share();

        // マップ画像読み込み
        AppCtx::AssetMgr().LoadAsync<asset::DivisionGraph>("map", "map.png");
    }

    Map::~Map()
    {
        if (m_prototype) return;
        // タイルコライダーを破棄
        if (auto* mgr = ColMgrPtr())
        {
            mgr->Destroy(colID);
        }

        AppCtx::AssetMgr().DeleteHandle<asset::DivisionGraph>("map");
    }

    void Map::Init()
    {
        // 試作ではJSONのワールド寸法をそのまま使用し、グリッド数への丸めを避ける。
        if (m_prototype)
        {
            Camera::Instance().SetWorldSize({m_phase1Config.WorldWidth, m_phase1Config.WorldHeight});
            return;
        }
        // ワールドサイズ設定
        Vector2f worldSize
        {
            m_mapInfo.mapSize.x * m_mapInfo.tileSize.x,
            m_mapInfo.mapSize.y * m_mapInfo.tileSize.y
        };
        gameSystem::Camera::Instance().SetWorldSize(worldSize);

        // 衝突マスク追加
        ColMgr().AddMask(colID, col2d::RECT, PLAYER);
    }

    void Map::OnAttach()
    {
        if (m_prototype) return;
        // 当たり判定生成(Loading中のコンストラクターから呼ばれる)
        col2d::ColliderDef def{};
        def.isActive = true;
        colID = ColMgr().CreateTileCollider(&def, m_infoFuture, "MapTip.csv", MyObjectTag());
    }

    void Map::Draw()
    {
        if (m_prototype)
        {
            // 画面内のグリッドだけ描画し、大きなワールドでも描画本数を抑える。
            auto& camera = Camera::Instance();
            const auto offset = camera.GetOffset();
            const auto screen = Window::GetWindowData()->SIZE;
            const int spacing = m_phase1Config.GridSize;
            const int startX = (std::max)(0, static_cast<int>(offset.x) / spacing * spacing);
            const int startY = (std::max)(0, static_cast<int>(offset.y) / spacing * spacing);
            const int endX = static_cast<int>((std::min)(m_phase1Config.WorldWidth, offset.x + screen.x));
            const int endY = static_cast<int>((std::min)(m_phase1Config.WorldHeight, offset.y + screen.y));
            for (int x = startX; x <= endX; x += spacing)
            {
                const auto p = camera.WorldToScreen(static_cast<float>(x), 0);
                DrawLine(static_cast<int>(p.x), 0, static_cast<int>(p.x),
                    static_cast<int>(screen.y), GetColor(45, 55, 65));
            }
            for (int y = startY; y <= endY; y += spacing)
            {
                const auto p = camera.WorldToScreen(0, static_cast<float>(y));
                DrawLine(0, static_cast<int>(p.y), static_cast<int>(screen.x),
                    static_cast<int>(p.y), GetColor(45, 55, 65));
            }
            const auto origin = camera.WorldToScreen(0, 0);
            const auto end = camera.WorldToScreen(m_phase1Config.WorldWidth - 1, m_phase1Config.WorldHeight - 1);
            DrawBox(static_cast<int>(origin.x), static_cast<int>(origin.y),
                static_cast<int>(end.x), static_cast<int>(end.y), GetColor(220, 220, 220), false);
            return;
        }
        // マップ未生成なら描画しない
        if (m_mapData.empty())
        {
            return;
        }

        // カメラオフセット取得
        Vector2f camOffset = gameSystem::Camera::Instance().GetOffset();
        const auto winSize = Window::GetWindowData()->SIZE;

        // 描画範囲算出
        const int startTileX = (std::max)(0, static_cast<int>(camOffset.x / m_mapInfo.tileSize.x));
        const int startTileY = (std::max)(0, static_cast<int>(camOffset.y / m_mapInfo.tileSize.y));
        const int endTileX = (std::min)(static_cast<int>(m_mapInfo.mapSize.x) - 1,
            static_cast<int>((camOffset.x + winSize.x) / m_mapInfo.tileSize.x) + 1);
        const int endTileY = (std::min)(static_cast<int>(m_mapInfo.mapSize.y) - 1,
            static_cast<int>((camOffset.y + winSize.y) / m_mapInfo.tileSize.y) + 1);

        // 描画範囲設定
        rangeX = { static_cast<size_t>(startTileX), static_cast<size_t>(endTileX) };
        rangeY = { static_cast<size_t>(startTileY), static_cast<size_t>(endTileY) };

        // 描画ループ
        for (size_t i = rangeY.first; i <= rangeY.second; i++)
        {
            for (size_t j = rangeX.first; j <= rangeX.second; j++)
            {
                // チャンクに沿ったキーを生成
                auto key = (static_cast<uint64_t>(i / m_mapInfo.chunkSize.y) << 32) | static_cast<uint64_t>(j / m_mapInfo.chunkSize.x);

                // ローカルインデックスでタイル取得
                const size_t localY = i % m_mapInfo.chunkSize.y;
                const size_t localX = j % m_mapInfo.chunkSize.x;
                const size_t localIndex = localY * m_mapInfo.chunkSize.x + localX;

                // 座標に沿ったハンドル取得
                auto mapIt = m_mapData.find(key);
                if (mapIt == m_mapData.end()) continue;
                int handle = AppCtx::AssetMgr().Fetch<asset::DivisionGraph>()->GetHandle("map", mapIt->second.at(localIndex));

                // チャンクに沿った座標に描画
                float x = j * m_mapInfo.tileSize.x;
                float y = i * m_mapInfo.tileSize.y;
                Vector2f camPos = gameSystem::Camera::Instance().WorldToScreen({ x, y });
                DrawGraph(static_cast<int>(camPos.x), static_cast<int>(camPos.y), handle, true);
            }
        }
    }

    void Map::CalcDrawRange(int pos, std::pair<size_t, size_t>& range, size_t length)
    {
        // 範囲算出
        if (length == 0)
        {
            range = {0, 0};
            return;
        }
        const int maxIndex = static_cast<int>(length) - 1;
        range.first  = static_cast<size_t>(std::clamp(pos - 5, 0, maxIndex));
        range.second = static_cast<size_t>(std::clamp(pos + 5, 0, maxIndex));
    }
}
