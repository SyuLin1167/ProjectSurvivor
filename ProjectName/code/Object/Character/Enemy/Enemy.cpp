module;
#include<DxLib.h>

module Object.Enemy;
import MyLib.Math.Vector2;
import GameSystem.Camera;

using namespace math;

namespace object
{
    Enemy::Enemy()
        :id()
        , m_health(MAX_HEALTH)
        , m_damage(1.0f)
    {
        m_pos = { 16 * 30,16 * 50 };
    }

    Enemy::~Enemy()
    {
        if (auto* mgr = ColMgrPtr())
        {
            mgr->Destroy(id);
        }
    }
    
    void Enemy::Init()
    {
        // コライダー作成
        col2d::ColliderDef def{};
        def.type = col2d::Type::STATIC;
        def.localPos = m_pos;
        def.isActive = true;
        id = ColMgr().CreateCircleCollider(&def, 16.0f, MyObjectTag());

        // 衝突イベント登録
        col2d::ContactListener listener;
        listener.when = [&](const col2d::ContactInfo&) { return true; };
        listener.event = [&]() {m_health -= m_damage * m_deltaTime; };
        ColMgr().AddEvent(id, col2d::MakeKey(col2d::RECT, ObjectTag::PLAYER), listener);
    }

    void Enemy::Update()
    {
        if(m_health <= 0.0f)
        {
            m_state = MainState::DEAD;
        }
    }

    void Enemy::Draw()
    {
        const auto sp = gameSystem::Camera::Instance().WorldToScreen(m_pos);
        DrawCircle(static_cast<int>(sp.x), static_cast<int>(sp.y), 16, GetColor(255, 150, 0), TRUE);
        DrawBox(static_cast<int>(sp.x) - 16, static_cast<int>(sp.y) - 32, static_cast<int>(sp.x) + (16 * m_health), static_cast<int>(sp.y) - 28, GetColor(255, 0, 0), true);
    }
}
