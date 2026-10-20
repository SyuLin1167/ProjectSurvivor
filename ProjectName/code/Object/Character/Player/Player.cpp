module;
#include <fstream>
#include <DxLib.h>
#include <json.hpp>

module Object.Player;

import Phase1.Config;
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
            // 承認値は中心座標。既存RectColliderに合わせて左上へ変換する。
            pData.size = { m_phase1Config.PlayerSize, m_phase1Config.PlayerSize };
            pData.pos = { m_phase1Config.PlayerCenterX - m_phase1Config.PlayerSize / 2,
                          m_phase1Config.PlayerCenterY - m_phase1Config.PlayerSize / 2 };
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
            id = ColMgr().CreateRectCollider(&def, pData.size, MyObjectTag());
            // 空き地では衝突マスクを使わず、Updateで矩形全体をマップ境界に制限する。
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

    void Player::Update()
    {
        if (m_prototype)
        {
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
        const auto p = Camera::Instance().WorldToScreen(m_pos);
        DrawBox(static_cast<int>(p.x), static_cast<int>(p.y),
            static_cast<int>(p.x + pData.size.x), static_cast<int>(p.y + pData.size.y),
            GetColor(40, 140, 255), true);
        DrawString(static_cast<int>(p.x + 10), static_cast<int>(p.y + 6),
            "P", GetColor(255, 255, 255));
        DrawFormatString(16, 40, GetColor(255, 255, 255),
            "P center: (%.2f, %.2f) u   speed: %.0f u/s",
            m_cameraCenter.x, m_cameraCenter.y, pData.moveSpeed.x);
    }
}
