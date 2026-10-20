export module Component.MoveWithKey;

import Component;
export import MyLib.Math.Vector2;

export using namespace math;

export namespace component
{
    export class MoveWithKey final :public ComponentBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="owner">所有者</param>
        MoveWithKey(object::GameObject* owner);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~MoveWithKey() = default;

        /// <summary>
        /// 水平移動の設定
        /// </summary>
        /// <param name="leftKey">左へ移動するためのキー</param>
        /// <param name="rightKey">右へ移動するためのキー</param>
        /// <param name="moveSpeed">移動速度</param>
        void SetHorizontal(const int leftKey, const int rightKey, float moveSpeed);

        /// <summary>
        /// 垂直移動の設定
        /// </summary>
        /// <param name="upKey">上へ移動するためのキー</param>
        /// <param name="downKey">下へ移動するためのキー</param>
        /// <param name="moveSpeed">移動速度</param>
        void SetVertical(const int upKey, const int downKey, float moveSpeed);

        // Alternative keys are ORed per direction, so duplicate presses do not accelerate.
        void SetAlternativeKeys(int left, int right, int up, int down)
        {
            m_altLeft = left; m_altRight = right;
            m_altUp = up; m_altDown = down;
        }

        void SetNormalizeDiagonal(bool enabled) { m_normalizeDiagonal = enabled; }

        /// <summary>
        /// 更新
        /// </summary>
        /// <param name="deltaTime">デルタタイム</param>
        void Update(const float& deltaTime) override;

        /// <summary>
        ///　X軸移動が可能かどうか
        /// </summary>
        /// <returns>移動可否情報</returns>
        const bool CanMoveHorizontal()
        {
            return m_canMoveX;
        }

        /// <summary>
        /// Y軸移動が可能かどうか
        /// </summary>
        /// <returns>移動可否情報</returns>
        const bool CanMoveVertical()
        {
            return m_canMoveY;
        }

    private:
        int m_altLeft = -1;
        int m_altRight = -1;
        int m_altUp = -1;
        int m_altDown = -1;
        bool m_normalizeDiagonal = false;
        int m_leftKey;
        int m_rightKey;
        int m_upKey;
        int m_downKey;
        Vector2f m_moveSpeed;
        bool m_canMoveX;
        bool m_canMoveY;
    };
}
