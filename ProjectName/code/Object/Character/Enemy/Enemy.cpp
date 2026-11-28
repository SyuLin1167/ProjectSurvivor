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
    Enemy::Enemy(Vector2f center, const combat::EnemyConfig& config) : m_config(config), id{}
    {
        m_pos = center;
        m_health = config.Health;
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
        id = ColMgr().CreateCircleCollider(&def, m_config.Radius, MyObjectTag());
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
                    const float step = m_config.TravelDistance(m_moveTime, m_deltaTime);
                    m_velocity = direction * (std::min)(distance, step);
                    m_moveTime += m_deltaTime;
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
        if (!base->TakeDamage(m_config.contactDamage)) return;
        // 接触通知はLateUpdateで一度だけ消費し、連続接触の通常端数だけを持ち越す。
        m_contactRemaining = m_config.contactInterval +
            (m_wasTouching && m_contactRemaining > -m_config.contactInterval ? m_contactRemaining : 0.0);
    }

    void Enemy::Draw()
    {
        auto& camera = gameSystem::Camera::Instance();
        // 移動・衝突・攻撃は画面外でも更新し、完全に見えない個体の描画呼び出しだけを省く。
        if (!m_config.VisualBounds(m_pos).AABB(camera.VisibleRect())) return;
        const auto sp = camera.WorldToScreen(m_pos);
        if (m_config.look != combat::EnemyConfig::Look::Classic)
        {
            if (m_config.look == combat::EnemyConfig::Look::Boss)
            {
                // 通常敵より大きな赤い円と内側の標識。全体をVisualBounds内へ収める。
                const int x = int(sp.x), y = int(sp.y), radius = int(m_config.Radius);
                DrawCircle(x, y, radius, GetColor(165, 35, 45), TRUE);
                DrawCircle(x, y, radius - 4, GetColor(255, 180, 55), FALSE);
                DrawString(x - 16, y - 8, "BOSS", GetColor(255, 255, 255));
            }
            else if (m_config.look == combat::EnemyConfig::Look::Charger)
            {
                // 追加素材なしでも識別できる紫の菱形。加速中は中心を白くし、32px範囲を守る。
                const int x = int(sp.x), y = int(sp.y);
                const auto color = GetColor(210, 80, 245);
                DrawTriangle(x, y - 16, x + 16, y, x, y + 16, color, TRUE);
                DrawTriangle(x, y - 16, x - 16, y, x, y + 16, color, TRUE);
                unsigned int core = GetColor(65, 25, 95);
                if (m_config.IsCharging(m_moveTime)) core = GetColor(255, 255, 255);
                DrawCircle(x, y, 5, core, TRUE);
            }
            else
            {
                // 既存3種は共有シートから描画し、突進型の図形描画とは同時に実行しない。
                auto sprite = alphaSprites::Sprite::Normal;
                if (m_config.look == combat::EnemyConfig::Look::Runner) sprite = alphaSprites::Sprite::Runner;
                if (m_config.look == combat::EnemyConfig::Look::Heavy) sprite = alphaSprites::Sprite::Heavy;
                alphaSprites::Draw(sprite, int(sp.x) - 16, int(sp.y) - 16, 32);
            }
            return;
        }
        DrawCircle(static_cast<int>(sp.x), static_cast<int>(sp.y), static_cast<int>(m_config.Radius), GetColor(220, 60, 100), TRUE);
        DrawString(static_cast<int>(sp.x - m_config.Radius), static_cast<int>(sp.y - m_config.Radius * 2), "ENEMY", GetColor(255, 255, 255));
    }
}
