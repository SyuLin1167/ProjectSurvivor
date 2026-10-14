module;
#include <DxLib.h>

module Scene.Result;

import <memory>;
import Scene.Result;
import Scene.Title;
import MyLib.KeyStatus;
import AppContext;
import Asset.Graph;

namespace scene
{
    Result::Result()
    {
        // 画像読み込み
        gameSystem::AppCtx::AssetMgr().LoadAsync<asset::Graph>("Result", "Result.png");
    }

    SceneCmd Result::Update()
    {
        // Rキーでリトライ（タイトルへ）
        if (input::KeyStatus::CheckKey(keyType.R, ON_PRESS))
        {
            return CmdReplace{ [](){ return std::make_shared<Title>(); } };
        }

        // 継続
        return std::monostate{};
    }

    void Result::Draw()
    {
        // 背景描画
        DrawGraph(0, 0, gameSystem::AppCtx::AssetMgr().Fetch<asset::Graph>()->GetHandle("Result"), FALSE);
        DrawFormatString(850, 950, GetColor(0, 0, 0), "CLEAR! Rキーでタイトルへ");
    }
}