module;
#include <algorithm>
#include <cmath>
#include <stdexcept>

export module Scene.StageState;

export namespace scene
{
    /// <summary>更新後に一度だけ発行するステージ終了要求。中断には勝敗を付けない。</summary>
    enum class StageEnd { None, Success, Defeat, Aborted };

    /// <summary>終了画面へ値で渡す結果。ゲームオブジェクトの寿命に依存しない。</summary>
    struct StageResult
    {
        StageEnd outcome = StageEnd::None;
        float health = 100;
        float progress = 0; // 0〜1。到達前の表示丸めは行わない。
    };

    /// <summary>一回のプレイの進行と終了確定を管理する。</summary>
    class StageState
    {
    public:
        /// <summary>開始・終端座標から進行0、結果未決で生成する。</summary>
        /// <param name="startX">拠点中心の開始X（u）。</param>
        /// <param name="endX">開始Xより大きい有限の終端X（u）。</param>
        StageState(float startX, float endX) : m_startX(startX), m_endX(endX)
        {
            if (!std::isfinite(startX) || !std::isfinite(endX) || endX <= startX)
                throw std::invalid_argument("StageState: invalid endpoints");
        }

        /// <summary>接触・LateUpdate後の値で、敗北→成功→中断の順に一度だけ確定する。</summary>
        /// <param name="centerX">更新後の拠点中心X（u）。</param>
        /// <param name="health">更新後の拠点HP。</param>
        /// <param name="abort">同フレームにRによる中断が要求された場合true。</param>
        /// <returns>今回初めて確定した終了要求。継続時と確定済みの再呼出しはNone。</returns>
        StageEnd Advance(float centerX, float health, bool abort)
        {
            if (Finished()) return StageEnd::None;
            m_distance = std::clamp(centerX - m_startX, 0.0f, m_endX - m_startX);
            m_result.progress = m_distance / (m_endX - m_startX);
            m_result.health = (std::max)(health, 0.0f);
            if (health <= 0) m_result.outcome = StageEnd::Defeat;
            else if (centerX >= m_endX) m_result.outcome = StageEnd::Success;
            else if (abort) m_result.outcome = StageEnd::Aborted;
            return m_result.outcome;
        }

        /// <summary>確定後にゲーム更新を停止すべきか返す。</summary>
        /// <returns>成功・敗北・中断のいずれかが確定済みならtrue。</returns>
        bool Finished() const { return m_result.outcome != StageEnd::None; }
        /// <summary>現在または終了時の結果値を返す。</summary>
        /// <returns>外部参照を含まない結果のコピー。</returns>
        StageResult ResultValue() const { return m_result; }
        /// <summary>開始地点から終端までの範囲に制限した進行距離を返す。</summary>
        /// <returns>進行距離（u）。</returns>
        float Distance() const { return m_distance; }
        /// <summary>終端までの残距離を返す。</summary>
        /// <returns>0以上の残距離（u）。</returns>
        float RemainingDistance() const { return m_endX - m_startX - m_distance; }

    private:
        float m_startX;
        float m_endX;
        float m_distance = 0;
        StageResult m_result{};
    };
}
