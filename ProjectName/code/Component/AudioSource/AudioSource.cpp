module;

#include <DxLib.h>

module Component.AudioSource;

import AppContext;
import Asset.Sound;
import Component;
import Asset.SoundDef;

using namespace gameSystem;
using namespace asset;

namespace component
{
    AudioSource::AudioSource(object::GameObject* owner)
        : ComponentBase(owner)
        , m_states()
    {
        // 処理なし
    }

    AudioSource::~AudioSource()
    {
        m_states.clear();
    }

    void AudioSource::Play(const std::string& soundName)
    {
        // 未登録の場合は停止状態で登録
        if (m_states.find(soundName) == m_states.end())
        {
            m_states[soundName] = SoundState::STOP;
        }

        // サウンドハンドルがない場合は再生しない
        auto sound = AppCtx::AssetMgr().Fetch<Sound>()->GetSound(soundName);
        if (sound.handle == -1)
        {
            return;
        }

        // 音量設定
        ChangeVolumeSoundMem(sound.soundDef.volume, sound.handle);

        // SE(単発)の場合は複製ハンドルを再生
        if (sound.soundDef.soundType == SOUNDTYPE_SE_ONCE)
        {
            int handle = DuplicateSoundMem(sound.handle);
            PlaySoundMem(handle, DX_PLAYTYPE_BACK);

            // 再生終了後に自動削除
            SetPlayFinishDeleteSoundMem(TRUE, handle);
            return;
        }

        // サウンド再生
        if (m_states[soundName] == SoundState::PAUSE)
        {
            PlaySoundMem(sound.handle, DX_PLAYTYPE_LOOP, false);
        }
        else if(m_states[soundName] == SoundState::STOP)
        {
            PlaySoundMem(sound.handle, DX_PLAYTYPE_LOOP);
        }
        m_states[soundName] = SoundState::PLAY;
    }

    void AudioSource::Pause(const std::string& soundName)
    {
        // サウンドがない場合は一時停止しない
        if (m_states.find(soundName) == m_states.end())
        {
            return;
        }

        // サウンドハンドルがない場合は一時停止しない
        auto sound = AppCtx::AssetMgr().Fetch<Sound>()->GetSound(soundName);
        if (sound.handle == -1)
        {
            return;
        }

        // サウンド一時停止
        StopSoundMem(sound.handle);
        m_states.at(soundName) = SoundState::PAUSE;
    }

    void AudioSource::Stop(const std::string& soundName)
    {
        // サウンドがない場合は停止しない
        if (m_states.find(soundName) == m_states.end())
        {
            return;
        }

        // サウンドハンドルがない場合は停止しない
        auto sound = AppCtx::AssetMgr().Fetch<Sound>()->GetSound(soundName);
        if (sound.handle == -1)
        {
            return;
        }

        // サウンド停止
        StopSoundMem(sound.handle);
        m_states.at(soundName) = SoundState::STOP;
    }
}

