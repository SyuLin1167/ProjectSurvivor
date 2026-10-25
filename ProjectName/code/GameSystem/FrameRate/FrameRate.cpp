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
        CalcFrameRateImpl();
    }

    void FrameRate::CalcFrameRateImpl()
    {
        //デルタタイムを算出
        m_nowTime = GetNowHiPerformanceCount();
        // 時刻差が負になった場合も全利用者へ非負の秒数を渡し、逆向きの更新を防ぐ。
        m_deltaTime = (std::max)(0.0f, (m_nowTime - m_prevTime) / MICRO_SEC);
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