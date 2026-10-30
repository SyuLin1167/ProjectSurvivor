module;
#include <DxLib.h>

module Object.MovingBase;
import Object.BaseMotion;
import MyLib.File.FileSystem;
import GameSystem.Camera;
import GameSystem.CameraGeometry;
import GameSystem.Window;
import MyLib.Shape.Rect;
import Collider;
import <algorithm>;
import <cmath>;

namespace object
{
    MovingBase::MovingBase()
    {
        m_config = phase1::Config::FromJson(gameSystem::AppCtx::FileSystem().jsonIO.Load(
            gameSystem::AppCtx::FileSystem().GetDataDir() / "Phase1Config"));
    }

    MovingBase::~MovingBase()
    {
        if (auto* manager = ColMgrPtr()) manager->Destroy(m_id);
    }

    void MovingBase::Init()
    {
        // 承認値は中心、RectColliderは左上。Pと同じ座標契約を使用する。
        const auto size = m_config.BaseSize;
        m_pos = m_config.BaseCenter - size.Half();
        col2d::ColliderDef def{};
        def.localPos = m_pos;
        def.isActive = true;
        def.isTrigger = false;
        m_id = ColMgr().CreateRectCollider(&def, size, MyObjectTag());
        // P・敵側だけに拠点マスクを登録し、拠点の前進を衝突で止めない。
    }

    void MovingBase::Update()
    {
        m_velocity = phase1::BaseDisplacement(Center().x,
            m_config.TrialEndX, m_config.BaseSpeed, m_deltaTime);
        ColMgr().GetCollider(m_id)->SetVelocity(m_velocity);
        // 座標はここでは足さず、既存ColliderManager::Stepに一度だけ反映させる。
    }

    bool MovingBase::TakeDamage(float damage)
    {
        const auto* collider = ColMgrPtr() ? ColMgr().GetCollider(m_id) : nullptr;
        if ((m_state & MainState::DEAD) || !collider || !collider->GetColliderDef()->isActive ||
            m_health <= 0 || !std::isfinite(damage) || damage <= 0) return false;
        m_health = (std::max)(0.0f, m_health - damage);
        return true;
    }

    void MovingBase::LateUpdate()
    {
        m_pos = ColMgr().GetCollider(m_id)->GetColliderDef()->localPos;
    }

    void MovingBase::Draw()
    {
        const auto size = m_config.BaseSize;
        const auto screen = gameSystem::Window::GetWindowData()->SIZE;
        const shape::Rect viewport{{0, 0}, {static_cast<float>(screen.x), static_cast<float>(screen.y)}};
        const shape::Rect baseRect{gameSystem::Camera::Instance().WorldToScreen(m_pos), size};
        const auto marker = cameraGeometry::BaseMarker(baseRect, viewport);
        // 描画だけを省く。Update/LateUpdateと進行値・距離の表示は画面外でも継続する。
        if (!marker.offscreen)
        {
            DrawBox(static_cast<int>(baseRect.Left()), static_cast<int>(baseRect.Top()),
                static_cast<int>(baseRect.Right()), static_cast<int>(baseRect.Bottom()),
                GetColor(255, 155, 40), true);
            DrawString(static_cast<int>(baseRect.Left() + 8), static_cast<int>(baseRect.Top() + 24),
                "BASE ->", GetColor(0, 0, 0));
        }
    }
}
