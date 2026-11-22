module;
#include <DxLib.h>
export module Object.ExperienceDrops;
import MyLib.Math.Vector2;
import MyLib.Shape.Circle;
import GameSystem.Camera;
import GameSystem.Window;
import <vector>;
import <algorithm>;
import <cstdint>;

export namespace object
{
    /// <summary>試行内の経験値・吸引中の粒を値で所有する。Colliderや敵への参照は持たない。</summary>
    class ExperienceDrops
    {
        /// <summary>個別登録や仮想関数を必要としない経験値の表示データ。</summary>
        struct Drop
        {
            math::Vector2f center; // ワールド中心（u）。
            uint64_t value = 1; // 回収時に加算するXP。
            bool attracted = false; // 一度吸引された後は範囲外でも追従を続ける。
        };
        std::vector<Drop> m_drops;
        uint64_t m_total = 0; // この試行で取得済みのXP。新しいPlayで0に戻る。
    public:
        static constexpr size_t Capacity = 256;
        static constexpr float MagnetRadius = 160, PickupRadius = 20, Speed = 480; // u、u、u/s。

        /// <summary>最大粒数を事前確保し、生成中の再確保を避ける。</summary>
        ExperienceDrops() { m_drops.reserve(Capacity); }

        /// <summary>撃破通知を一度消費した呼び出し元が、撃破位置へ1XPを置く。</summary>
        /// <param name="center">敵の撃破位置（u）。</param>
        void Add(math::Vector2f center)
        {
            if (m_drops.size() < Capacity)
            {
                m_drops.push_back({center});
                return;
            }
            // 上限でも報酬を失わないよう、最寄りの既存粒へ合算する。距離比較は平方根不要。
            auto nearest = std::min_element(m_drops.begin(), m_drops.end(), [center](const auto& a, const auto& b) {
                return (a.center - center).LengthSq() < (b.center - center).LengthSq();
            });
            ++nearest->value;
        }

        /// <summary>吸引範囲へ入った粒を移動し、取得距離に達した値を一度だけ累計へ移す。</summary>
        /// <param name="dt">共通ゲーム経過秒。停止中は0。</param>
        /// <param name="playerCenter">移動後のプレイヤー中心（u）。</param>
        void Update(float dt, math::Vector2f playerCenter)
        {
            if (dt <= 0) return; // 非アクティブ停止中は取得も進めない。
            const shape::Circle magnet{playerCenter, MagnetRadius};
            const shape::Circle pickup{playerCenter, PickupRadius};
            std::erase_if(m_drops, [&](Drop& drop) {
                // 取得円内なら同座標も含め即座に加算し、正規化へ進まない。
                if (pickup.IsInside(drop.center))
                {
                    m_total += drop.value;
                    return true;
                }
                if (!drop.attracted) drop.attracted = magnet.IsInside(drop.center);
                if (!drop.attracted) return false;

                const auto delta = playerCenter - drop.center;
                const float step = Speed * dt;
                // 行き過ぎを防ぐ。Norm自体にもゼロ対策があるが、ここでは取得が先に成立する。
                if (step * step >= delta.LengthSq()) drop.center = playerCenter;
                else drop.center += delta.Norm() * step;
                if (!pickup.IsInside(drop.center)) return false;
                m_total += drop.value;
                return true;
            });
        }

        /// <summary>画面内の経験値を水色の小さな円で描く。</summary>
        void DrawField() const
        {
            const auto& camera = gameSystem::Camera::Instance();
            const auto view = camera.VisibleRect();
            for (const auto& drop : m_drops)
            {
                if (!view.AABB({drop.center - math::Vector2f{5, 5}, {10, 10}})) continue;
                const auto point = camera.WorldToScreen(drop.center);
                DrawCircle(int(point.x), int(point.y), 5, GetColor(90, 220, 255), TRUE);
            }
        }

        /// <summary>特殊アイテム表示の上に、取得済み経験値を表示する。</summary>
        void DrawStatus() const
        {
            const int y = int(gameSystem::Window::GetWindowData()->SIZE.y) - 58;
            DrawFormatString(18, y, GetColor(90, 220, 255), "XP: %llu", static_cast<unsigned long long>(m_total));
        }
        /// <summary>取得済み経験値を返す。</summary>
        /// <returns>現在の試行のXP。</returns>
        uint64_t Total() const { return m_total; }
        /// <summary>フィールドに残る粒の数を返す。</summary>
        /// <returns>合算後の表示個数。</returns>
        size_t Count() const { return m_drops.size(); }
    };
}
