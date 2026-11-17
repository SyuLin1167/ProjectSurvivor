module;
#include<DxLib.h>

module Object.Enemy;
import MyLib.Math.Vector2;
import GameSystem.Camera;
import Asset.AlphaSprites;
import <algorithm>;
import GameSystem.FrameRate;
import <cmath>;

using namespace math;

namespace object
{
    Enemy::Enemy(Vector2f center, const combat::EnemyConfig& config) : id{}
    {
        m_pos = center;
        m_health = config.Health;
        m_radius = config.Radius;
        m_speed = config.Speed;
        m_look = config.look;

    }

    void Enemy::GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj)
    {
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
        def.type = col2d::Type::DYNAMIC;
        def.localPos = m_pos;
        def.isActive = true;
        def.isTrigger = false;
        def.contactAfterMove = true;
        id = ColMgr().CreateCircleCollider(&def, m_radius, MyObjectTag());
        m_colliderCreated = true;

        // 敵から拠点への片方向マスク。P/敵同士は通過し、拠点側は押し戻さない。
        ColMgr().AddMask(id, col2d::RECT, ObjectTag::BASE);
        col2d::ContactListener contact;
        contact.event = [this]() { m_touching = true; };
        ColMgr().AddEvent(id, col2d::MakeKey(col2d::RECT, ObjectTag::BASE), contact);
    }

    bool Enemy::CanBeAttacked() const
    {
        const auto* collider = m_colliderCreated && ColMgrPtr() ? ColMgr().GetCollider(id) : nullptr;
        return !(m_state & MainState::DEAD) &&
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
        m_touching = false;
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
        m_pos = ColMgr().GetCollider(id)->GetColliderDef()->localPos;
        // 全Colliderの帰還通知後に各敵の有効打を一度だけ精算する。
        // 非接触中も時計を進め、期限到達と接触が揃った更新でだけ攻撃する。
        if (gameSystem::FrameRate::Countdown(m_contactRemaining, m_deltaTime, true) && m_touching) OnBaseContact();
        if (!m_touching) m_contactRemaining = (std::max)(0.0, m_contactRemaining);
        m_wasTouching = m_touching;
    }

    void Enemy::OnBaseContact()
    {
        const auto base = m_base.lock();
        // 接触の事実はColliderイベントだけから受け取る。撃破済み/対象消失時は被害を出さない。
        if (!CanBeAttacked() || !base || (base->NowState() & MainState::DEAD)) return;
        if (!base->TakeDamage(CONTACT_DAMAGE)) return;
        // 接触通知はLateUpdateで一度だけ消費し、連続接触の通常端数だけを持ち越す。
        m_contactRemaining = CONTACT_INTERVAL +
            (m_wasTouching && m_contactRemaining > -CONTACT_INTERVAL ? m_contactRemaining : 0.0);
    }

    void Enemy::Draw()
    {
        const auto sp = gameSystem::Camera::Instance().WorldToScreen(m_pos);
        if (m_look != combat::EnemyConfig::Look::Classic)
        {
            auto sprite = alphaSprites::Sprite::Normal;
            if (m_look == combat::EnemyConfig::Look::Runner) sprite = alphaSprites::Sprite::Runner;
            if (m_look == combat::EnemyConfig::Look::Heavy) sprite = alphaSprites::Sprite::Heavy;
            alphaSprites::Draw(sprite, int(sp.x) - 16, int(sp.y) - 16, 32);
            return;
        }
        DrawCircle(static_cast<int>(sp.x), static_cast<int>(sp.y), static_cast<int>(m_radius), GetColor(220, 60, 100), TRUE);
        DrawString(static_cast<int>(sp.x - m_radius), static_cast<int>(sp.y - m_radius * 2), "ENEMY", GetColor(255, 255, 255));
    }
}
