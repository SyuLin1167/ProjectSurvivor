export module GameSystem.Camera;

import MyLib.Math.Vector2;
import GameSystem.Window;
import Object.GameObject;

using namespace math;

export namespace gameSystem
{
    /// <summary>
    /// 2Dカメラ
    /// </summary>
    export class Camera final
    {
    public:
        /// <summary>
        /// インスタンス生成
        /// </summary>
        /// <returns>カメラインスタンス</returns>
        static Camera& Instance()
        {
            static Camera inst;
            return inst;
        }

        /// <summary>
        /// 追尾ターゲット設定
        /// </summary>
        /// <param name="target">追尾対象のゲームオブジェクト</param>
        /// <param name="centered">trueなら対象座標へ中央追従する。境界制限は維持する。</param>
        void SetTarget(Vector2f* target, bool centered = false)
        {
            m_target = target;
            m_centered = centered;
        }

        /// <summary>
        /// 追尾ターゲット解除(自分が登録したターゲットの場合のみ)
        /// </summary>
        /// <param name="target">解除するターゲット</param>
        void ClearTarget(const Vector2f* target)
        {
            if (m_target == target)
            {
                m_target = nullptr;
            }
        }

        /// <summary>
        /// ワールド全体サイズ設定
        /// </summary>
        /// <param name="worldSize">ワールドサイズ</param>
        void SetWorldSize(const Vector2f& worldSize= Vector2f(1920.0f, 1080.0f))
        {
            m_worldSize = worldSize;
            ClampPosition();
        }

        /// <summary>
        /// 更新
        /// </summary>
        void Update();

        /// <summary>
        /// 現在のオフセット
        /// </summary>
        /// <returns>ワールド座標から画面座標への変換時に減算するオフセット（u）。</returns>
        Vector2f GetOffset() const
        {
            return m_pos;
        }

        /// <summary>
        /// ワールド→スクリーン座標変換
        /// </summary>
        /// <param name="worldPos">ワールド座標</param>
        /// <returns>スクリーン座標</returns>
        Vector2f WorldToScreen(const Vector2f& worldPos) const
        {
            return worldPos - m_pos;
        }

        /// <summary>
        /// ワールド→スクリーン座標変換
        /// </summary>
        /// <param name="x">ワールド座標X</param>
        /// <param name="y">ワールド座標Y</param>
        /// <returns>スクリーン座標</returns>
        Vector2f WorldToScreen(float x, float y) const
        {
            return Vector2f(x, y) - m_pos;
        }

    private:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Camera();

        /// <summary>
        /// 位置クランプ
        /// </summary>
        void ClampPosition();

        Vector2f* m_target;           // 追尾ターゲット
        Vector2f m_pos;              // カメラ位置（オフセット）
        Vector2f m_worldSize;        // ワールド全体サイズ
        float m_followStrength;      // 追尾強度
        bool m_centered = false;     // 試作では中央へ直接追従し、フレーム単位の補間を行わない。
    };
}
