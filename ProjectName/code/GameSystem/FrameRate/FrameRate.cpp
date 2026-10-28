module;
#include <DxLib.h>

module GameSystem.FrameRate;
import <algorithm>;

namespace gameSystem
{
    FrameRate::FrameRate()
        : m_startTime(GetNowHiPerformanceCount())
        , m_nowTime(m_startTime)
        , m_prevTime(m_startTime)
        , m_deltaTime()
        , m_fps(FPS_60)
        , m_frameCount()
    {
        CalcFrameRateImpl(true);
    }

    void FrameRate::CalcFrameRateImpl(bool active)
    {
        //デルタタイムを算出
        m_nowTime = GetNowHiPerformanceCount();
        // 実時間FPSは保持し、ゲーム時間だけ停止・復帰・上限を制御する。
        m_deltaTime = m_simulationTime.Advance((m_nowTime - m_prevTime) / MICRO_SEC, active);
        m_prevTime = m_nowTime;

        //fps算出
        if (m_frameCount == FPS_60)
        {
            m_fps = MICRO_SEC / ((m_nowTime - m_startTime) / FPS_60);
            m_startTime = m_nowTime;
            m_frameCount = 0;
        }
        m_frameCount++;
    }

#ifdef _DEBUG
    void FrameRate::DrawFrameRate()
    {
        printfDx("deltaTime: %f", FrameRate::GetDeltaTime());
        printfDx("fps: %3.1f", Instance().m_fps);
    }
#endif // _DEBUG
}