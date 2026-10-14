export module Component.AudioSource;

import <string>;
import <unordered_map>;

import Component;
import Asset.SoundDef;
import Object.GameObject;

using namespace asset;

/// <summary>
/// コンポーネント関連
/// </summary>
export namespace component
{
    /// <summary>
    /// サウンドを操作するクラス
    /// </summary>
    export class AudioSource final : public ComponentBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        AudioSource() = default;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="owner">所有者</param>
        AudioSource(object::GameObject* owner);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~AudioSource();

        /// <summary>
        /// 再生
        /// </summary>
        /// <param name="soundName">再生するサウンド</param>
        void Play(const std::string& soundName);

        /// <summary>
        /// 一時停止
        /// </summary>
        /// <param name="soundName">一時停止するサウンド</param>
        void Pause(const std::string& soundName);

        /// <summary>
        /// 停止
        /// </summary>
        /// <param name="soundName">停止するサウンド</param>
        void Stop(const std::string& soundName);

    private:
        std::unordered_map<std::string, SoundState> m_states; // 再生情報群
    };
}
