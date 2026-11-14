module;
#include <DxLib.h>
export module Asset.AlphaSprites;
import Asset.Graph;
import AppContext;

export namespace alphaSprites
{
    /// <summary>仮素材シートの上段を左から参照する。地面のみ下段を使う。</summary>
    enum class Sprite { Player, Normal, Runner, Heavy, Ground };

    /// <summary>初回のAlpha開始時に共有画像を読み込み、解放は既存AssetManagerに任せる。</summary>
    inline void Load()
    {
        auto* graph = gameSystem::AppCtx::AssetMgr().Fetch<asset::Graph>();
        if (graph->GetHandle("alpha_combat") < 0)
            gameSystem::AppCtx::AssetMgr().LoadAsync<asset::Graph>("alpha_combat", "alpha_combat.png");
    }

    /// <summary>シート内の指定矩形を画面座標へ描画する。</summary>
    /// <param name="sprite">表示する仮素材。</param>
    /// <param name="x">左端の画面座標。</param>
    /// <param name="y">上端の画面座標。</param>
    /// <param name="size">表示する一辺（px）。</param>
    inline void Draw(Sprite sprite, int x, int y, int size)
    {
        const bool ground = sprite == Sprite::Ground;
        const int handle = gameSystem::AppCtx::AssetMgr().Fetch<asset::Graph>()->GetHandle("alpha_combat");
        DrawRectExtendGraph(x, y, x + size, y + size, ground ? 0 : int(sprite) * 32,
            ground ? 32 : 0, 32, 32, handle, TRUE);
    }

    /// <summary>下段の96×64の荷車を描く。</summary>
    /// <param name="x">荷車の左端（px）。</param>
    /// <param name="y">荷車の上端（px）。</param>
    inline void DrawCaravan(int x, int y)
    {
        const int handle = gameSystem::AppCtx::AssetMgr().Fetch<asset::Graph>()->GetHandle("alpha_combat");
        DrawRectGraph(x, y, 32, 32, 96, 64, handle, TRUE);
    }
}
