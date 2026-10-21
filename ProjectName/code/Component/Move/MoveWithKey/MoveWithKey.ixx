export module Component.MoveWithKey;

import Component;
export import MyLib.Math.Vector2;

export using namespace math;

export namespace component
{
    /// <summary>キー入力を所有オブジェクトの移動量へ変換するコンポーネント。</summary>
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

        /// <summary>代替キーを方向ごとにORで扱い、同時押しによる加速を防ぐ。</summary>
        void SetAlternativeKeys(int left, int right, int up, int down)
        {
            m_altLeft = left; m_altRight = right;
            m_altUp = up; m_altDown = down;
        }

        /// <summary>斜め入力時の速度正規化を有効・無効にする。</summary>
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
