module;
#include <algorithm>
#include <cmath>

export module GameSystem.FrameRate;

import <memory>;

/// <summary>
/// システム関連
/// </summary>
export namespace gameSystem
{
    /// <summary>開始・停止復帰の時間持ち越しを防ぐゲーム時計。</summary>
    class SimulationTime final
    {
    public:
        /// <summary>次回の経過時間を破棄し、新しいプレイの時間原点にする。</summary>
        void Reset() { m_wasActive = false; }
        /// <summary>DEC-03の時間を算出する。停止・復帰直後は0、通常時は最大1/30秒。</summary>
        /// <param name="elapsed">実時間の経過秒数。</param>
        /// <param name="active">ウィンドウがアクティブならtrue。</param>
        /// <returns>全シミュレーションで共有する経過秒数。無効値は0。</returns>
        float Advance(float elapsed, bool active)
        {
            const bool running = active && m_wasActive;
            m_wasActive = active;
            return running && std::isfinite(elapsed) ? std::clamp(elapsed, 0.0f, 1.0f / 30.0f) : 0.0f;
        }
    private:
        bool m_wasActive = false;
    };

    /// <summary>
    /// フレームレートの算出
    /// </summary>
    export class FrameRate final
    {
    public:
        /// <summary>
        /// コピー禁止令
        /// </summary>
        FrameRate(const FrameRate&) = delete;
        FrameRate(FrameRate&&) = delete;

        /// <summary>
        /// 代入禁止令
        /// </summary>
        FrameRate& operator=(const FrameRate&) = delete;
        FrameRate& operator=(FrameRate&&) = delete;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~FrameRate() = default;

        /// <summary>
        /// フレームレートを算出
        /// </summary>
        /// <param name="active">ウィンドウがアクティブならtrue。</param>
        static void CalcFrameRate(bool active = true)
        {
            Instance().CalcFrameRateImpl(active);
        }

        /// <summary>
        /// デルタタイム取得
        /// </summary>
        /// <returns>ゲーム用経過秒数（0〜1/30秒）。停止・復帰直後は0。実時間FPSとは分離する。</returns>
        static const float& GetDeltaTime()
        {
            return Instance().m_deltaTime;
        }

        /// <summary>読み込み・初期化の所要時間を新しいプレイへ持ち越さない。</summary>
        static void ResetSimulationTime()
        {
            Instance().m_simulationTime.Reset();
            Instance().m_deltaTime = 0;
        }

#ifdef _DEBUG
        /// <summary>
        /// FPS(フレームレート)描画
        /// </summary>
        static void DrawFrameRate();
#endif // _DEBUG

    private:
        /// <summary>
        /// 自身のインスタンスを返す
        /// </summary>
        /// <returns>自身のインスタンス</returns>
        static FrameRate& Instance()
        {
            static FrameRate instance;
            return instance;
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        FrameRate();

        /// <summary>時刻差から非負の経過秒数と表示用FPSを更新する。</summary>
        /// <param name="active">ウィンドウがアクティブならtrue。</param>
        void CalcFrameRateImpl(bool active);

        SimulationTime m_simulationTime;

        inline static constexpr float MICRO_SEC = 1000000.0f;     //マイクロ秒
        inline static constexpr float FPS_60 = 60.0f;             //60fps

        long long m_startTime;                    //開始時刻
        long long m_nowTime;                      //現在時刻
        long long m_prevTime;                     //1フレーム前の時刻
        float m_deltaTime;                        //デルタタイム
        float m_fps;                              //FramePerSecond
        float m_frameCount;                       //フレームカウント
    };
}
