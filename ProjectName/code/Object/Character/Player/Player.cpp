module;
#include <DxLib.h>

module Object.Player;

import Scene.StageConfig;
import Object.Enemy;
import <algorithm>;
import GameSystem.FrameRate;
import Component.MovementMath;

import MyLib.KeyStatus;
import MyLib.Math.Vector2;
import GameSystem.Camera;
import Component.MoveWithKey;
import Asset.AlphaSprites;
import Collider;

using namespace gameSystem;
using namespace math;
using namespace col2d;

namespace object
{
    Player::Player(const stage::StageConfig& config, const combat::PlayerAttackConfig& attack)
        : id(), m_attack(attack), m_stageConfig(config)
    {
        // 承認値は中心座標。既存RectColliderに合わせて左上へ変換する。
        pData.size = { m_stageConfig.PlayerSize, m_stageConfig.PlayerSize };
        pData.pos = m_stageConfig.PlayerCenter - m_stageConfig.PlayerSize / 2;
        pData.moveSpeed = { m_stageConfig.PlayerSpeed, m_stageConfig.PlayerSpeed };
    }

    Player::~Player()
    {
        // カメラ追従を解除(自分が登録した場合のみ)
        gameSystem::Camera::Instance().ClearTarget(&m_cameraCenter);

        // コライダーを破棄(イベントも同時に削除される)
        if (auto* mgr = ColMgrPtr())
        {
            mgr->Destroy(id);
        }
    }

    void Player::Init()
    {
        // 初期位置設定
        m_pos = pData.pos;

        // 移動機能追加
        auto move = m_compMgr->AddComponent<component::MoveWithKey>(this);
        move->SetHorizontal(keyType.LEFT, keyType.RIGHT, pData.moveSpeed.x);

        move->SetVertical(keyType.UP, keyType.DOWN, pData.moveSpeed.y);
        move->SetAlternativeKeys(keyType.A, keyType.D, keyType.W, keyType.S);
        move->SetNormalizeDiagonal(true);
        m_cameraCenter = m_pos + pData.size.Half();
        Camera::Instance().SetTarget(&m_cameraCenter, true);
        Camera::Instance().Update();
        ColliderDef def{};
        def.localPos = m_pos;
        def.isActive = true;
        def.isTrigger = false;
        def.contactAfterMove = true;
        id = ColMgr().CreateRectCollider(&def, pData.size, MyObjectTag());
        // 拠点側は解決しない。移動後にPだけを押し戻し、拠点の自動前進を維持する。
        ColMgr().AddMask(id, col2d::RECT, ObjectTag::BASE);
    }

    void Player::GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj)
    {
        m_referenceObjects = std::move(referenceObj);
    }

    void Player::UpdateAutoAttack()
    {
        // 同一更新の撃破だけを渡し、非アクティブ時は強化時間を消費しない。
        m_killPosition.reset();
        FrameRate::Countdown(m_boostSeconds, m_deltaTime);
        m_hitEffect.Update(m_deltaTime);
        if (!FrameRate::Countdown(m_attackRemaining, m_deltaTime, true) || !m_referenceObjects) return;
        std::shared_ptr<Enemy> target;
        float nearest = m_attack.Range;
        // 1回の取得結果だけを使用する。永続的なshared_ptrで敵の破棄を妨げない。
        for (const auto& object : m_referenceObjects(ObjectTag::ENEMY))
        {
            const auto enemy = std::dynamic_pointer_cast<Enemy>(object);
            if (!enemy || !enemy->CanBeAttacked()) continue;
            const float distance = Center().Distance(enemy->Center());
            if (distance <= m_attack.Range && (!target || distance < nearest ||
                (distance == nearest && enemy->TargetId() < target->TargetId())))
            {
                target = enemy;
                nearest = distance;
            }
        }
        // 命中直前にも有効性と射程を確認。対象なしで攻撃時間を借金として蓄積しない。
        if (!target || !target->CanBeAttacked() || Center().Distance(target->Center()) > m_attack.Range ||
            !target->TakeDamage(m_attack.Damage * (m_boostSeconds > 0.000001 ? 1.5f : 1.0f)))
        {
            m_attackRemaining = 0;
            return;
        }
        if (m_stageConfig.alphaCombat && target->Health() <= 0)
            m_killPosition = target->Center();
        m_lastHit = target;
        m_lastHitCenter = target->Center();
        m_hitEffect.Hit(m_stageConfig.alphaCombat, target->Health() <= 0);
        // 通常の端数は保持し、大きな遅延の攻撃回数は捨てる。連射ループは設けない。
        m_attackRemaining = m_attack.Interval +
            (m_attackRemaining > -m_attack.Interval ? m_attackRemaining : 0.0);
    }

    void Player::Update()
    {
        UpdateAutoAttack();
        // 移動予定量だけを制限し、座標更新はColliderManager::Stepに一度だけ任せる。
        m_velocity.x = movement::BoundedDisplacement(m_pos.x, m_velocity.x,
            pData.size.x, m_stageConfig.WorldWidth);
        m_velocity.y = movement::BoundedDisplacement(m_pos.y, m_velocity.y,
            pData.size.y, m_stageConfig.WorldHeight);
        ColMgr().GetCollider(id)->SetVelocity(m_velocity);
    }

    void Player::LateUpdate()
    {
        // 座標更新
        m_pos = ColMgr().GetCollider(id)->GetColliderDef()->localPos;

        // 速度更新
        m_velocity = ColMgr().GetCollider(id)->GetVelocity();
        // 衝突更新後の実座標から追従中心を更新し、描画とカメラのずれを防ぐ。
        m_cameraCenter = m_pos + pData.size.Half();
    }

    void Player::Draw()
    {
        if (m_hitEffect.Active())
        {
            const auto target = std::dynamic_pointer_cast<Enemy>(m_lastHit.lock());
            const auto hit = Camera::Instance().WorldToScreen(target ? target->Center() : m_lastHitCenter);
            m_hitEffect.Draw(int(hit.x), int(hit.y));
        }
        const auto p = Camera::Instance().WorldToScreen(m_pos);
        if (m_stageConfig.alphaCombat)
        {
            alphaSprites::Draw(alphaSprites::Sprite::Player, int(p.x), int(p.y), int(pData.size.x));
            return;
        }
        DrawBox(static_cast<int>(p.x), static_cast<int>(p.y),
            static_cast<int>(p.x + pData.size.x), static_cast<int>(p.y + pData.size.y),
            GetColor(40, 140, 255), true);
        DrawString(static_cast<int>(p.x + 10), static_cast<int>(p.y + 6),
            "P", GetColor(255, 255, 255));
    }
}
