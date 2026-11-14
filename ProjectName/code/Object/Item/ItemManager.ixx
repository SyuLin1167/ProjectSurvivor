module;
#include <DxLib.h>
export module Object.CombatDrops;
import Object.Player;
import Object.MovingBase;
import Collider;
import Collider.ColliderManager;
import ObjectTag;
import <array>;
import <variant>;
import <utility>;
import MyLib.Math.Vector2;
import GameSystem.Camera;
import GameSystem.Window;
import Object.Item;
import Object.Repair;
import Object.Boost;
import <random>;
import <algorithm>;
import GameSystem.FrameRate;
import <cmath>;

export namespace object
{
    /// <summary>このPlayの戦利品と短い回収表示を管理する。敵への参照は保持しない。</summary>
    class CombatDrops
    {
        using Slot = std::variant<std::monostate, Repair, Boost>;
        /// <summary>占有中スロットの共通アイテムを参照する。</summary>
        /// <param name="slot">固定プールの1枠。</param>
        /// <returns>空きならnullptr。</returns>
        static Item* Get(Slot& slot)
        {
            if (auto* repair = std::get_if<Repair>(&slot)) return repair;
            return std::get_if<Boost>(&slot);
        }
        col2d::ColliderManager& manager;
        std::array<Slot, 8> items{}; // 通知先のアドレスは登録中変わらない。
        std::mt19937 rng{5002}; // 生成方向の乱数と分離し、同一撃破順で比較できる。
        math::Vector2f popupCenter{};
        float popupTime{}, recovered{};
    public:
        /// <summary>このPlayの衝突管理にだけアイテムを登録する。</summary>
        /// <param name="colliders">全スロットより長く存続するシーンの管理。</param>
        explicit CombatDrops(col2d::ColliderManager& colliders) : manager(colliders) {}
        CombatDrops(const CombatDrops&) = delete;
        CombatDrops& operator=(const CombatDrops&) = delete;

        /// <summary>撃破ごとに一度だけ抽選し、最大8個までフィールドに置く。</summary>
        /// <param name="center">撃破時の敵中心。</param>
        void Roll(math::Vector2f center)
        {
            const auto roll = std::uniform_int_distribution<int>(0, 3)(rng);
            if (roll >= 2) return;
            for (auto& slot : items)
            {
                if (Get(slot)) continue;
                if (roll == 0) slot.emplace<Repair>();
                else slot.emplace<Boost>();
                auto* item = Get(slot);
                item->center = center;

                // 移動後のプレイヤー矩形だけを検出し、押し戻しは行わない。
                col2d::ColliderDef def{};
                def.localPos = center;
                def.isActive = def.isTrigger = def.contactAfterMove = true;
                item->id = manager.CreateCircleCollider(&def, 10, ITEM);
                item->manager = &manager;
                manager.AddMask(item->id, col2d::RECT, PLAYER);
                col2d::ContactListener listener;
                listener.event = [item]() { item->touching = true; };
                manager.AddEvent(item->id, col2d::MakeKey(col2d::RECT, PLAYER), listener);
                return;
            }
        }

        /// <summary>円トリガーの通知を消費し、勝敗確定前の取得や多重適用を防ぐ。</summary>
        /// <param name="dt">共通ゲーム経過秒。</param>
        /// <param name="player">拾得するプレイヤー。</param>
        /// <param name="base">修理対象の拠点。</param>
        void Collect(float dt, Player& player, MovingBase& base)
        {
            gameSystem::FrameRate::Countdown(popupTime, dt);
            for (auto& slot : items)
            {
                auto* item = Get(slot);
                if (!item) continue;
                const bool touched = std::exchange(item->touching, false);
                if (dt <= 0) continue;
                if (gameSystem::FrameRate::Countdown(item->remaining, dt)) { slot.emplace<std::monostate>(); continue; }
                if (!touched) continue;
                const float effect = item->Apply(player, base);
                if (effect < 0) continue; // 満HPなら次の接触で再評価する。
                recovered = effect;
                popupCenter = item->center;
                popupTime = 0.7f;
                slot.emplace<std::monostate>();
            }
        }

        /// <summary>修理は緑の十字、強化は金色の矢印。拾得地点に短いリングを残す。</summary>
        void DrawField() const
        {
            auto& camera = gameSystem::Camera::Instance();
            for (const auto& item : items)
            {
                if (std::holds_alternative<std::monostate>(item)) continue;
                const auto* repair = std::get_if<Repair>(&item);
                const auto p = camera.WorldToScreen(repair ? repair->center : std::get<Boost>(item).center);
                const int x = int(p.x), y = int(p.y);
                const auto color = repair ? GetColor(100,255,150) : GetColor(255,205,60);
                DrawBox(x-10,y-10,x+10,y+10,GetColor(32,36,46),true);
                DrawLine(x,y-7,x,y+7,color,3);
                if (repair) DrawLine(x-7,y,x+7,y,color,3);
                else
                {
                    DrawLine(x-6,y,x,y-7,color,3);
                    DrawLine(x+6,y,x,y-7,color,3);
                }
            }
            if (popupTime > 0)
            {
                const auto p = camera.WorldToScreen(popupCenter);
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
            if (popupTime > 0 && recovered > 0)
                DrawFormatString(190,y,GetColor(100,255,150),"Repair +%.0f HP",recovered);
        }
    };
}
