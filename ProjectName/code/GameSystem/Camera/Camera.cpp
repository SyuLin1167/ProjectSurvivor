module GameSystem.Camera;

import <algorithm>;
import GameSystem.FrameRate;
import MyLib.Math.Vector2;
import GameSystem.Window;

using namespace math;

namespace gameSystem
{
    Camera::Camera()
        : m_target(nullptr)
        , m_pos(0.0f, 0.0f)
        , m_worldSize(0.0f, 0.0f)
        , m_followStrength(0.15f)
    {
        // 処理なし
    }

    void Camera::Update()
    {
        if (!m_target)
        {
            return;
        }

        // ウィンドウサイズ取得
        auto winSize = Window::GetWindowData()->SIZE;

        // スクロールするターゲットの座標
        Vector2f target = *m_target - winSize / 3;

        // 線形補間で追尾
        m_pos += (target - m_pos) * std::clamp(m_followStrength, 0.0f, 1.0f);

        ClampPosition();
    }

    void Camera::ClampPosition()
    {
        auto winSize = Window::GetWindowData()->SIZE;

        // マップサイズが未設定の場合はクランプ不可
        if (m_worldSize.x <= 0.0f || m_worldSize.y <= 0.0f)
        {
            m_pos.x = std::max(0.0f, m_pos.x);
            m_pos.y = std::max(0.0f, m_pos.y);
            return;
        }

        // カメラ位置をワールドサイズ内にクランプ
        float maxX = std::max(0.0f, m_worldSize.x - winSize.x);
        float maxY = std::max(0.0f, m_worldSize.y - winSize.y);

        // クランプ実行
        m_pos.x = std::clamp(m_pos.x, 0.0f, maxX);
        m_pos.y = std::clamp(m_pos.y, 0.0f, maxY);
    }
}