module;
#include <fstream>
#include <DxLib.h>
#include <json.hpp>

module Object.Player;

import Phase1.Config;
import Object.Enemy;
import <algorithm>;
import Component.MovementMath;

import MyLib.File.FileSystem;
import MyLib.KeyStatus;
import MyLib.Loading.LoadingContext;
import MyLib.Math.Vector2;
import GameSystem.Camera;
import Component.Jump;
import Component.MoveWithKey;
import Component.Animator;
import Component.AudioSource;
import Asset.Animation;
import Asset.Sound;
import Collider;

using json = nlohmann::json;
using namespace gameSystem;
using namespace math;
using namespace col2d;

namespace object
{
    Player::Player(bool prototype)
        :id()
        , m_prototype(prototype)
    {
        if (m_prototype)
        {
            // 既存JsonIOで試作値を読み込む。欠損・不正な設定は例外として報告する。
            m_phase1Config = phase1::Config::FromJson(AppCtx::FileSystem().jsonIO.Load(
                AppCtx::FileSystem().GetDataDir() / "Phase1Config"));
            m_attack = phase2::PlayerAttackConfig::FromJson(AppCtx::FileSystem().jsonIO.Load(
                AppCtx::FileSystem().GetDataDir() / "PlayerAttackConfig"));
            // 承認値は中心座標。既存RectColliderに合わせて左上へ変換する。
            pData.size = { m_phase1Config.PlayerSize, m_phase1Config.PlayerSize };
            pData.pos = m_phase1Config.PlayerCenter - m_phase1Config.PlayerSize / 2;
            pData.moveSpeed = { m_phase1Config.PlayerSpeed, m_phase1Config.PlayerSpeed };
            return;
        }
        // プレイヤー初期データ読み込み
        json data = AppCtx::FileSystem().jsonIO.Load(AppCtx::FileSystem().GetDataDir() / "PlayerData");
        pData.Input(data);

        //プレイヤー画像読み込み
        AppCtx::AssetMgr().Fetch<asset::Animation>()->CreateHandleAsync("Player", "player.png", pData.size.x, pData.size.y);

        // 歩行サウンド定義
        asset::SoundDef walkSoundDef{};
        walkSoundDef.name = AssetName::WALK;
        walkSoundDef.soundType = asset::SOUNDTYPE_SE_LOOP;
        walkSoundDef.volume = 100;

        // プレイヤーの歩行音読み込み
        AppCtx::AssetMgr().Fetch<asset::Sound>()->AddSound(walkSoundDef, "SE_Footsteps.wav");
    }

    Player::~Player()
    {
        // カメラ追従を解除(自分が登録した場合のみ)
        gameSystem::Camera::Instance().ClearTarget(&m_pos);
        gameSystem::Camera::Instance().ClearTarget(&m_cameraCenter);

        // コライダーを破棄(イベントも同時に削除される)
        if (auto* mgr = ColMgrPtr())
        {
            mgr->Destroy(id);
        }

        if (m_prototype) return;

        // プレイヤー画像削除
        AppCtx::AssetMgr().Fetch<asset::Animation>()->DeleteHandle("Player");

        // アニメーション削除
        AppCtx::AssetMgr().Fetch<asset::Animation>()->DeleteAnim(AssetName::IDLE);
        AppCtx::AssetMgr().Fetch<asset::Animation>()->DeleteAnim(AssetName::WALK);

        // プレイヤーの歩行音削除
        AppCtx::AssetMgr().Fetch<asset::Sound>()->DeleteHandle(AssetName::WALK);
    }

    void Player::Init()
    {
        // 初期位置設定
        m_pos = pData.pos;

        // カメラ追従ターゲット設定
        gameSystem::Camera::Instance().SetTarget(&m_pos);

        // 移動機能追加
        auto move = m_compMgr->AddComponent<component::MoveWithKey>(this);
        move->SetHorizontal(keyType.LEFT, keyType.RIGHT, pData.moveSpeed.x);

        if (m_prototype)
        {
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
            return;
        }

        // ジャンプ機能追加
        auto jump = m_compMgr->AddComponent<component::Jump>(
            this,
            pData.moveSpeed.y,
            std::bind(input::KeyStatus::CheckKey, keyType.SPACE, ON_PRESS)
        );

        // 当たり判定追加
        ColliderDef colDef{};
        colDef.localPos = m_pos;
        colDef.isActive = true;
        colDef.shouldCCD = true;
        id = ColMgr().CreateRectCollider(&colDef, pData.size, MyObjectTag());
        ColMgr().AddMask(id, col2d::CIRCLE, ObjectTag::ENEMY);

        // 衝突イベント追加
        ContactListener listener;
        listener.when = [&](const ContactInfo& info) { return info.normal == NORMAL_TOP; };
        listener.event = [&, jump]() { jump->CanJump();};
        ColMgr().AddEvent(id, MakeKey(TILE, ObjectTag::MAP), listener);

        // 待機アニメーション定義
        asset::AnimationDef idleAnimDef{};
        idleAnimDef.name = AssetName::IDLE;
        idleAnimDef.type = asset::AnimType::LOOP;
        idleAnimDef.animationSpeed = 4;
        idleAnimDef.endFrame = 4;
        idleAnimDef.size = pData.size;

        // 歩行アニメーション定義
        asset::AnimationDef walkAnimDef{};
        walkAnimDef = idleAnimDef;
        walkAnimDef.name = AssetName::WALK;
        walkAnimDef.startFrame = 5;
        walkAnimDef.endFrame = 8;

        // アニメーション追加
        AppCtx::AssetMgr().Fetch<asset::Animation>()->CreateAnimCategory("Player", { idleAnimDef,walkAnimDef });
        auto anim = m_compMgr->AddComponent<component::Animator>(this);
        anim->AddAnim(AssetName::IDLE);
        anim->AddAnim(AssetName::WALK);

        // サウンド再生コンポーネント追加
        m_compMgr->AddComponent<component::AudioSource>(this);
    }

    void Player::GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj)
    {
        if (m_prototype) m_referenceObjects = std::move(referenceObj);
    }

    void Player::UpdateAutoAttack()
    {
        m_hitDisplayRemaining = (std::max)(0.0f, m_hitDisplayRemaining - m_deltaTime);
        if (m_attackRemaining > 0) m_attackRemaining -= m_deltaTime;
        if (m_attackRemaining > 0.000001 || !m_referenceObjects) return;
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
            !target->TakeDamage(m_attack.Damage))
        {
            m_attackRemaining = 0;
            return;
        }
        m_lastHit = target;
        m_lastHitCenter = target->Center();
        m_hitDisplayRemaining = 0.15f; // 命中確認だけの短い表示。正式アニメーションではない。
        // 通常の端数は保持し、大きな遅延の攻撃回数は捨てる。連射ループは設けない。
        m_attackRemaining = m_attack.Interval +
            (m_attackRemaining > -m_attack.Interval ? m_attackRemaining : 0.0);
    }

    void Player::Update()
    {
        if (m_prototype)
        {
            UpdateAutoAttack();
            // 移動予定量だけを制限し、座標更新はColliderManager::Stepに一度だけ任せる。
            m_velocity.x = movement::BoundedDisplacement(m_pos.x, m_velocity.x,
                pData.size.x, m_phase1Config.WorldWidth);
            m_velocity.y = movement::BoundedDisplacement(m_pos.y, m_velocity.y,
                pData.size.y, m_phase1Config.WorldHeight);
            ColMgr().GetCollider(id)->SetVelocity(m_velocity);
            return;
        }
        // 速度反映
        ColMgr().GetCollider(id)->SetVelocity(m_velocity);

        if (m_velocity.x != 0)
        {
            m_compMgr->GetComponent<component::Animator>()->SwitchAnim(AssetName::WALK, AssetName::IDLE);
            m_compMgr->GetComponent<component::AudioSource>()->Play(AssetName::WALK);
        }
        else
        {
            m_compMgr->GetComponent<component::Animator>()->SwitchAnim(AssetName::IDLE, AssetName::WALK);
            m_compMgr->GetComponent<component::AudioSource>()->Stop(AssetName::WALK);
        }
    }

    void Player::LateUpdate()
    {
        // 座標更新
        m_pos = ColMgr().GetCollider(id)->GetColliderDef()->localPos;

        // 速度更新
        m_velocity = ColMgr().GetCollider(id)->GetVelocity();
        // 衝突更新後の実座標から追従中心を更新し、描画とカメラのずれを防ぐ。
        if (m_prototype) m_cameraCenter = m_pos + pData.size.Half();
    }

    void Player::Draw()
    {
        if (!m_prototype) return;
        if (m_hitDisplayRemaining > 0)
        {
            const auto target = std::dynamic_pointer_cast<Enemy>(m_lastHit.lock());
            const auto hit = Camera::Instance().WorldToScreen(target ? target->Center() : m_lastHitCenter);
            DrawCircle(static_cast<int>(hit.x), static_cast<int>(hit.y), 22, GetColor(255, 255, 255), false);
        }
        const auto p = Camera::Instance().WorldToScreen(m_pos);
        DrawBox(static_cast<int>(p.x), static_cast<int>(p.y),
            static_cast<int>(p.x + pData.size.x), static_cast<int>(p.y + pData.size.y),
            GetColor(40, 140, 255), true);
        DrawString(static_cast<int>(p.x + 10), static_cast<int>(p.y + 6),
            "P", GetColor(255, 255, 255));
    }
}
