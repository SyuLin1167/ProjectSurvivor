module;
#include <DxLib.h>
export module Object.CombatDrops;
import Object.Player;
import Object.MovingBase;
import MyLib.Shape.Rect;
import MyLib.Math.Vector2;
import GameSystem.Camera;
import GameSystem.Window;
import <vector>;
import <random>;
import <algorithm>;
import <cmath>;

export namespace object
{
    /// <summary>このPlayの戦利品と短い回収表示を管理する。敵への参照は保持しない。</summary>
    class CombatDrops
    {
        struct Item { math::Vector2f center; bool repair; float remaining = 20.0f; };
        std::vector<Item> items;
        std::mt19937 rng{5002}; // 生成方向の乱数と分離し、同一撃破順で比較できる。
        Item popup{};
        float popupTime{}, recovered{};
    public:
        /// <summary>撃破ごとに一度だけ抽選し、最大8個までフィールドに置く。</summary>
        /// <param name="center">撃破時の敵中心。</param>
        void Roll(math::Vector2f center)
        {
            const auto roll = std::uniform_int_distribution<int>(0, 3)(rng);
            if (roll >= 2 || items.size() >= 8) return;
            items.push_back({center, roll == 0}); // 修理25%、攻撃強化25%。
        }

        /// <summary>探索シールドと同じ矩形接触判定で即時適用し、成功した品だけ除去する。</summary>
        /// <param name="dt">共通ゲーム経過秒。</param>
        /// <param name="player">拾得するプレイヤー。</param>
        /// <param name="base">修理対象の拠点。</param>
        void Collect(float dt, Player& player, MovingBase& base)
        {
            if (dt <= 0) return;
            popupTime = (std::max)(0.0f, popupTime - dt);
            const shape::Rect actor{player.Center() - player.pData.size.Half(), player.pData.size};
            std::erase_if(items, [&](Item& item) {
                // 遠方や満HPで残った品が上限を占め続けないよう、ゲーム時間で片付ける。
                item.remaining -= dt;
                if (item.remaining <= 0) return true;
                const math::Vector2f size{20, 20};
                if (!actor.AABB(shape::Rect{item.center - size.Half(), size})) return false;
                if (item.repair)
                {
                    const float healed = base.Repair();
                    if (healed <= 0) return false;
                    recovered = healed;
                }
                else player.BoostAttack();
                popup = item;
                popupTime = 0.7f;
                return true;
            });
        }

        /// <summary>修理は緑の十字、強化は金色の矢印。拾得地点に短いリングを残す。</summary>
        void DrawField() const
        {
            auto& camera = gameSystem::Camera::Instance();
            for (const auto& item : items)
            {
                const auto p = camera.WorldToScreen(item.center);
                const int x = int(p.x), y = int(p.y);
                const auto color = item.repair ? GetColor(100,255,150) : GetColor(255,205,60);
                DrawBox(x-10,y-10,x+10,y+10,GetColor(32,36,46),true);
                DrawLine(x,y-7,x,y+7,color,3);
                if (item.repair) DrawLine(x-7,y,x+7,y,color,3);
                else
                {
                    DrawLine(x-6,y,x,y-7,color,3);
                    DrawLine(x+6,y,x,y-7,color,3);
                }
            }
            if (popupTime > 0)
            {
                const auto p = camera.WorldToScreen(popup.center);
                DrawCircle(int(p.x),int(p.y),int(22-popupTime*15),GetColor(255,255,255),false);
            }
        }

        /// <summary>既存HUDと探索案内を避けて画面下部に取得結果と残り時間を表示する。</summary>
        /// <param name="boost">通常攻撃強化の残りゲーム秒。</param>
        void DrawStatus(double boost) const
        {
            const int y = int(gameSystem::Window::GetWindowData()->SIZE.y) - 32;
            if (boost > 0.000001)
                DrawFormatString(18,y,GetColor(255,205,60),"ATK x1.5  %ds",int(std::ceil(boost)));
            if (popupTime > 0 && popup.repair)
                DrawFormatString(190,y,GetColor(100,255,150),"Repair +%.0f HP",recovered);
        }
    };
}
