module;
#include<DxLib.h>

module Object.Enemy;
import MyLib.Math.Vector2;
import GameSystem.Camera;
import <algorithm>;
import <cmath>;

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

    Enemy::Enemy(Vector2f center, const phase2::EnemyConfig& config) : Enemy()
    {
        m_prototype = true;
        m_pos = center;
        m_health = config.Health;
        m_radius = config.Radius;
        m_speed = config.Speed;

    }

    void Enemy::GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj)
    {
        if (!m_prototype) return;
        const auto bases = referenceObj(ObjectTag::BASE);
        m_base = bases.empty() ? std::shared_ptr<MovingBase>{} :
            std::dynamic_pointer_cast<MovingBase>(bases.front());
    }

    Enemy::~Enemy()
    {
        if (auto* mgr = ColMgrPtr(); mgr && m_colliderCreated)
        {
            mgr->Destroy(id);
        }
    }
    
    void Enemy::Init()
    {
        // コライダー作成
        col2d::ColliderDef def{};
        def.type = m_prototype ? col2d::Type::DYNAMIC : col2d::Type::STATIC;
        def.localPos = m_pos;
        def.isActive = true;
        def.isTrigger = false;
        def.contactAfterMove = m_prototype;
        id = ColMgr().CreateCircleCollider(&def, m_radius, MyObjectTag());
        m_colliderCreated = true;

        if (m_prototype)
        {
            // 敵から拠点への片方向マスク。P/敵同士は通過し、拠点側は押し戻さない。
            ColMgr().AddMask(id, col2d::RECT, ObjectTag::BASE);
            col2d::ContactListener contact;
            contact.event = [this]() { OnBaseContact(); };
            ColMgr().AddEvent(id, col2d::MakeKey(col2d::RECT, ObjectTag::BASE), contact);
            return;
        }

        // 衝突イベント登録
        col2d::ContactListener listener;
        listener.when = [&](const col2d::ContactInfo&) { return true; };
        listener.event = [&]() {m_health -= m_damage * m_deltaTime; };
        ColMgr().AddEvent(id, col2d::MakeKey(col2d::RECT, ObjectTag::PLAYER), listener);
    }

    bool Enemy::CanBeAttacked() const
    {
        const auto* collider = m_colliderCreated && ColMgrPtr() ? ColMgr().GetCollider(id) : nullptr;
        return m_prototype && m_health > 0 && !(m_state & MainState::DEAD) &&
            collider && collider->GetColliderDef()->isActive;
    }

    bool Enemy::TakeDamage(float damage)
    {
        if (!CanBeAttacked() || !std::isfinite(damage) || damage <= 0) return false;
        m_health = (std::max)(0.0f, m_health - damage);
        if (m_health == 0)
        {
            // Player更新は敵更新・Stepより先。配列を走査中に削除せず、撃破だけ即時確定する。
            m_state |= MainState::DEAD;
            auto* collider = ColMgr().GetCollider(id);
            collider->GetColliderDef()->isActive = false;
            collider->SetVelocity({});
        }
        return true;
    }

    void Enemy::Update()
    {
        if(m_health <= 0.0f)
            m_state = MainState::DEAD;

        if (!m_prototype) return;
        m_touching = false;
        m_attackedThisFrame = false;
        if (m_contactRemaining > 0) m_contactRemaining -= m_deltaTime;
        m_velocity = {};
        // DEAD対象は参照が残っていても追跡しない。対象なしでは前フレームの変位も消す。
        if (!(m_state & MainState::DEAD))
            if (const auto base = m_base.lock(); base && !(base->NowState() & MainState::DEAD))
            {
                const auto delta = base->Center() - m_pos;
                const float distance = base->Center().Distance(m_pos);
                if (distance > 0.0f)
                {
                    const auto direction = delta.Norm();
                    // Colliderへ渡す値はu/sではなく変位(u)。中心の通過による往復を防ぐ。
                    const float step = m_speed * m_deltaTime;
                    m_velocity = direction * (std::min)(distance, step);
                }
            }
        // 円の座標更新は既存Stepで一度だけ行う。画面内外は参照しない。
        ColMgr().GetCollider(id)->SetVelocity(m_velocity);
    }

    void Enemy::LateUpdate()
    {
        if (m_prototype)
        {
            m_pos = ColMgr().GetCollider(id)->GetColliderDef()->localPos;
            if (!m_touching) m_contactRemaining = (std::max)(0.0, m_contactRemaining);
            m_wasTouching = m_touching;
        }
    }

    void Enemy::OnBaseContact()
    {
        const auto base = m_base.lock();
        // 接触の事実はColliderイベントだけから受け取る。撃破済み/対象消失時は被害を出さない。
        if (!CanBeAttacked() || !base || (base->NowState() & MainState::DEAD)) return;
        m_touching = true;
        if (!m_attackedThisFrame && m_contactRemaining <= 0.000001)
        {
            if (base->TakeDamage(CONTACT_DAMAGE))
            {
                m_attackedThisFrame = true;
                // 連続接触の通常端数のみ保持。長い遅延でも1フレーム1発、非接触中の蓄積なし。
                m_contactRemaining = CONTACT_INTERVAL +
                    (m_wasTouching && m_contactRemaining > -CONTACT_INTERVAL ? m_contactRemaining : 0.0);
            }
        }
    }

    void Enemy::Draw()
    {
        const auto sp = gameSystem::Camera::Instance().WorldToScreen(m_pos);
        if (m_prototype)
        {
            DrawCircle(static_cast<int>(sp.x), static_cast<int>(sp.y), static_cast<int>(m_radius), GetColor(220, 60, 100), TRUE);
            DrawString(static_cast<int>(sp.x - m_radius), static_cast<int>(sp.y - m_radius * 2), "ENEMY", GetColor(255, 255, 255));
            return;
        }
        DrawCircle(static_cast<int>(sp.x), static_cast<int>(sp.y), 16, GetColor(255, 150, 0), TRUE);
        DrawBox(static_cast<int>(sp.x) - 16, static_cast<int>(sp.y) - 32, static_cast<int>(sp.x) + (16 * m_health), static_cast<int>(sp.y) - 28, GetColor(255, 0, 0), true);
    }
}
