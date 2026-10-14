export module UI.UIBase;

import <vector>;
import <functional>;
import MyLib.Math.Vector2;

using namespace math;

/// <summary>
/// UI関連
/// </summary>
export namespace ui
{
    /// <summary>
    /// UI要素の基底クラス
    /// </summary>
    export class UIBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        UIBase(): m_pos{ 0.0f, 0.0f }, m_size{ 0.0f, 0.0f }, m_visible{ true } {}

        /// <summary>
        /// デストラクタ
        /// </summary>
        virtual ~UIBase() = default;

        /// <summary>
        /// 位置設定
        /// </summary>
        /// <param name="pos">位置</param>
        void SetPosition(const Vector2f& pos)
        {
            m_pos = pos;
        }

        /// <summary>
        /// サイズ設定
        /// </summary>
        /// <param name="size">サイズ</param>
        void SetSize(const Vector2f& size)
        {
            m_size = size;
        }

        /// <summary>
        /// 表示設定
        /// </summary>
        /// <param name="visible">表示状態</param>
        void SetVisible(bool visible)
        {
            m_visible = visible;
        }

        /// <summary>
        /// 位置取得
        /// </summary>
        /// <returns>位置</returns>
        const Vector2f& GetPosition() const
        {
            return m_pos;
        }

        /// <summary>
        /// サイズ取得
        /// </summary>
        /// <returns>サイズ</returns>
        const Vector2f& GetSize() const
        {
            return m_size;
        }

        /// <summary>
        /// 表示状態取得
        /// </summary>
        /// <returns>表示状態</returns>
        bool IsVisible() const
        {
            return m_visible;
        }

        /// <summary>
        /// 更新処理
        /// </summary>
        virtual void Update() {}

        /// <summary>
        /// 描画処理
        /// </summary>
        virtual void Draw() {}

    protected:
        Vector2f m_pos;     // 位置
        Vector2f m_size;    // サイズ
        bool m_visible;     // 表示状態
    };
}
