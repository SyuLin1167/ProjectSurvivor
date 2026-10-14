export module Component.Animator;

import <string>;
import <memory>;
import <unordered_map>;

import Component;
import Asset.AnimationDef;
import Object.GameObject;

using namespace asset;

/// <summary>
/// コンポーネント関連
/// </summary>
export namespace component
{
    /// <summary>
    /// アニメーションを操作するクラス
    /// </summary>
    export class Animator final : public ComponentBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Animator() = default;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="owner">所有者</param>
        Animator(object::GameObject* owner);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Animator();

        /// <summary>
        /// アニメーション追加
        /// </summary>
        /// <param name="animName"></param>
        void AddAnim(const std::string& animName);

        /// <summary>
        /// 再生
        /// </summary>
        /// <param name="animName">再生するアニメーション</param>
        void PlayAnim(const std::string& animName);

        /// <summary>
        /// アニメーション画像を反転
        /// </summary>
        /// <param name="animName">対象のアニメーション</param>
        void TurnAnimGraph(const std::string& animName);

        /// <summary>
        /// 一時停止
        /// </summary>
        /// <param name="animName">一時停止するアニメーション</param>
        void PauseAnim(const std::string& animName);

        /// <summary>
        /// 停止
        /// </summary>
        /// <param name="animName">停止するアニメーション</param>
        void StopAnim(const std::string& animName);

        /// <summary>
        /// 不可視化
        /// </summary>
        /// <param name="animName">不可視化するアニメーション</param>
        void EnableAnim(const std::string& animName)
        {
            auto& playState = m_states.at(animName);
            playState.enable = true;
        }

        /// <summary>
        /// アニメーション切り替え
        /// </summary>
        /// <param name="issueName">再生対象のアニメーション</param>
        /// <param name="targetName"切り替え対象のアニメーション></param>
        void SwitchAnim(const std::string& issueName, const std::string& targetName);

        /// <summary>
        /// 更新処理
        /// </summary>
        /// <param name="deltaTime">デルタタイム</param>
        void Update(const float& deltaTime) override;

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw() override;

    private:
        /// <summary>
        /// アニメーションの再生情報
        /// </summary>
        struct AnimPlayState
        {
            float nowFrame = 0;
            AnimState animState = AnimState::STOP;
            bool isReverce = false;
            bool turnFrag = false;
            bool enable = false;
        };

        std::unordered_map<std::string, AnimPlayState> m_states; // 再生情報郡
    };
}