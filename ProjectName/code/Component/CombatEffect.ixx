module;
#include <DxLib.h>
export module Component.CombatEffect;
import <algorithm>;

export namespace component
{
    /// <summary>所有者のゲーム時間で進む短い命中・撃破・被害表示。対象の寿命には依存しない。</summary>
    class CombatEffect
    {
        enum class Style { Ring, Slash, Fragments, Damage };
        Style style{};
        float elapsed{};
        float duration{};
    public:
        /// <summary>命中表示を初期化する。</summary>
        /// <param name="alpha">Alphaの演出を使用するか。</param>
        /// <param name="killed">今回の命中で撃破したか。</param>
        void Hit(bool alpha, bool killed)
        {
            style = alpha ? (killed ? Style::Fragments : Style::Slash) : Style::Ring;
            elapsed = 0;
            duration = 0.15f;
        }

        /// <summary>拠点の実HP減少を短い赤枠で示す。</summary>
        void Hurt()
        {
            style = Style::Damage;
            elapsed = 0;
            duration = 0.22f;
        }

        /// <summary>表示時間をゲーム更新と同期させる。</summary>
        /// <param name="dt">ゲーム内の経過秒。</param>
        void Update(float dt) { elapsed = (std::min)(duration, elapsed + dt); }

        /// <summary>表示期間内かを確認する。</summary>
        /// <returns>描画対象ならtrue。</returns>
        bool Active() const { return elapsed < duration; }

        /// <summary>命中位置または拠点の矩形に演出を重ねる。</summary>
        /// <param name="x">命中中心または拠点左端の画面X。</param>
        /// <param name="y">命中中心または拠点上端の画面Y。</param>
        /// <param name="right">被害表示の右端。</param>
        /// <param name="bottom">被害表示の下端。</param>
        void Draw(int x, int y, int right = 0, int bottom = 0) const
        {
            if (!Active()) return;
            const int radius = 17 + int(elapsed * 120);
            switch (style)
            {
            case Style::Ring:
                DrawCircle(x, y, 22, GetColor(255, 255, 255), false);
                break;
            case Style::Slash:
                DrawLine(x - radius, y + radius, x + radius, y - radius, GetColor(255, 242, 182), 4);
                DrawCircle(x, y, radius, GetColor(245, 142, 75), false);
                break;
            case Style::Fragments:
                // 敵が消えた後も、命中中心から四方へ破片を広げる。
                for (int sx : {-1, 1})
                    for (int sy : {-1, 1})
                        DrawLine(x + sx * radius, y + sy * radius,
                            x + sx * (radius - 8), y + sy * (radius - 8), GetColor(255, 191, 73), 3);
                break;
            case Style::Damage:
                const int pad = 2 + int(elapsed * 32);
                DrawBox(x - pad, y - pad, right + pad, bottom + pad, GetColor(255, 69, 58), false);
                break;
            }
        }
    };
}
