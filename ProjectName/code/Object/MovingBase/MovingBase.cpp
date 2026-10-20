module;
#include <DxLib.h>

module Object.MovingBase;
import Object.BaseMotion;
import MyLib.File.FileSystem;
import GameSystem.Camera;
import Collider;

namespace object
{
    MovingBase::MovingBase(bool& trialFinished) : m_trialFinished(trialFinished)
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
        const math::Vector2f size{m_config.BaseWidth, m_config.BaseHeight};
        m_pos = math::Vector2f{m_config.BaseCenterX, m_config.BaseCenterY} - size.Half();
        col2d::ColliderDef def{};
        def.localPos = m_pos;
        def.isActive = true;
        m_id = ColMgr().CreateRectCollider(&def, size, MyObjectTag());
        // マスクなしでPと相互通過。基盤のtrigger/地形経路へ入らない。
    }

    void MovingBase::Update()
    {
        m_velocity = phase1::BaseDisplacement(m_pos.x + m_config.BaseWidth / 2,
            m_config.TrialEndX, m_config.BaseSpeed, m_deltaTime);
        ColMgr().GetCollider(m_id)->SetVelocity(m_velocity);
        // 座標はここでは足さず、既存ColliderManager::Stepに一度だけ反映させる。
    }

    void MovingBase::LateUpdate()
    {
        m_pos = ColMgr().GetCollider(m_id)->GetColliderDef()->localPos;
        if (m_pos.x + m_config.BaseWidth / 2 >= m_config.TrialEndX)
            m_trialFinished = true;
    }

    void MovingBase::Draw()
    {
        const auto p = gameSystem::Camera::Instance().WorldToScreen(m_pos);
        DrawBox(static_cast<int>(p.x), static_cast<int>(p.y),
            static_cast<int>(p.x + m_config.BaseWidth), static_cast<int>(p.y + m_config.BaseHeight),
            GetColor(255, 155, 40), true);
        DrawString(static_cast<int>(p.x + 8), static_cast<int>(p.y + 24),
            "BASE ->", GetColor(0, 0, 0));
        // 画面外でも進行値を観測する試作用表示。方向・距離インジケータはIssue #6。
        DrawFormatString(16, 64, GetColor(255, 190, 80),
            "BASE center: (%.2f, %.2f) u   speed: %.0f u/s   trial end X: %.0f",
            m_pos.x + m_config.BaseWidth / 2, m_pos.y + m_config.BaseHeight / 2,
            m_config.BaseSpeed, m_config.TrialEndX);
    }
}
