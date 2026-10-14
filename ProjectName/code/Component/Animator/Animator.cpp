module;

#include <DxLib.h>

module Component.Animator;

import AppContext;
import Asset.Animation;
import GameSystem.Camera;

using namespace gameSystem;

namespace component
{
    Animator::Animator(object::GameObject* owner)
        :ComponentBase(owner)
        , m_states()
    {
        // 処理なし
    }

    Animator::~Animator()
    {
        m_states.clear();
    }

    void Animator::AddAnim(const std::string& animName)
    {
        // 管理下に置くアニメーションを新規追加
        if (m_states.find(animName) == m_states.end())
        {
            AnimPlayState state;
            auto& anim = AppCtx::AssetMgr().Fetch<asset::Animation>()->GetAnim(animName).second;
            state.nowFrame = anim.startFrame;
            m_states.emplace(animName, state);
        }
    }

    void Animator::PlayAnim(const std::string& animName)
    {
        auto& playState = m_states.at(animName);
        playState.enable = false;
        if (playState.animState == asset::AnimState::PLAY)
        {
            return;
        }
        playState.animState = asset::AnimState::PLAY;
    }

    void Animator::TurnAnimGraph(const std::string& animName)
    {
        m_states.at(animName).turnFrag = !m_states.at(animName).turnFrag;
    }

    void Animator::PauseAnim(const std::string& animName)
    {
        auto& playState = m_states.at(animName);
        if (playState.animState == asset::AnimState::PAUSE)
        {
            return;
        }
        playState.animState = asset::AnimState::PAUSE;
    }

    void Animator::StopAnim(const std::string& animName)
    {
        auto& playState = m_states.at(animName);
        if (playState.animState == asset::AnimState::STOP)
        {
            return;
        }
        playState.animState = asset::AnimState::STOP;

        auto& anim = AppCtx::AssetMgr().Fetch<asset::Animation>()->GetAnim(animName).second;
        playState.nowFrame = anim.startFrame;
    }

    void Animator::SwitchAnim(const std::string& issueName, const std::string& targetName)
    {
        StopAnim(targetName);
        EnableAnim(targetName);
        PlayAnim(issueName);
    }

    void Animator::Update(const float& deltaTime)
    {
        // 現在所持している再生情報を一斉更新
        for (auto& [animName, playState] : m_states)
        {
            // 再生中以外は未更新にする
            if (playState.animState != asset::AnimState::PLAY)
            {
                continue;
            }

            // フレーム更新
            auto& anim = AppCtx::AssetMgr().Fetch<asset::Animation>()->GetAnim(animName).second;
            playState.nowFrame += (!playState.isReverce ? anim.animationSpeed : -anim.animationSpeed) * deltaTime;

            // 再生終了した場合
            if (playState.nowFrame >= anim.endFrame)
            {
                // ループは開始地点、ピンポンは反転用フラグを反転し終了地点へフレームを移動
                if (anim.type == asset::AnimType::LOOP)
                {
                    playState.nowFrame = anim.startFrame;
                }
                else if (anim.type == asset::AnimType::PINGPONG)
                {
                    playState.isReverce = !playState.isReverce;
                    playState.nowFrame = anim.endFrame;
                }
            }
            else if (playState.nowFrame <= anim.startFrame)
            {
                // ピンポンは反転用フラグを反転し開始地点へフレームを移動
                if(anim.type == asset::AnimType::PINGPONG)
                {
                    playState.isReverce = !playState.isReverce;
                    playState.nowFrame = anim.startFrame;
                }
            }
        }
    }

    void Animator::Draw()
    {
        // 現在所持している再生情報を一斉描画
        for (auto& [animName, playState] : m_states)
        {
            if (playState.enable)
            {
                continue;
            }
            const math::Vector2f sp = Camera::Instance().WorldToScreen(m_owner->AccessPos().NowPos());
            auto& anim = AppCtx::AssetMgr().Fetch<asset::Animation>()->GetAnim(animName);
            auto pos = sp + anim.second.size.Half();
            DrawRotaGraphF(pos.x, pos.y, 1, 0, anim.first[(size_t)playState.nowFrame], true, playState.turnFrag);
        }
    }
}