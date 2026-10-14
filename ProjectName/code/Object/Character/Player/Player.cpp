module;
#include <fstream>
#include <DxLib.h>
#include <json.hpp>

module Object.Player;

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
    Player::Player()
        :id()
    {
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

        // コライダーを破棄(イベントも同時に削除される)
        if (auto* mgr = ColMgrPtr())
        {
            mgr->Destroy(id);
        }

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
    }

    void Player::Draw()
    {
        // 処理なし
    }
}
