module GameSystem.Camera;

import <algorithm>;
import GameSystem.FrameRate;
import GameSystem.CameraGeometry;
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

        if (m_centered)
        {
            m_pos = *m_target - winSize / 2;
            ClampPosition();
            return;
        }

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

        // 描画と同じviewportで制限し、小さいマップでも負のoffsetにしない。
        m_pos = cameraGeometry::ClampOffset(m_pos, m_worldSize,
            {static_cast<float>(winSize.x), static_cast<float>(winSize.y)});
    }
}
