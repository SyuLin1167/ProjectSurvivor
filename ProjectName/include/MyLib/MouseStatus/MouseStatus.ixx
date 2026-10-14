export module MyLib.MouseStatus;

import <unordered_map>;
import MyLib.Math.Vector2;

export constexpr int8_t CLICK_ON_RELEASE = 0x0001;   // 未入力時
export constexpr int8_t CLICK_RELEASING  = 0x0002;   // 未入力中
export constexpr int8_t CLICK_ON_PRESS   = 0x0004;   // 入力時
export constexpr int8_t CLICK_PRESSING   = 0x0008;   // 入力中

using namespace math;

/// <summary>
/// 入力関連（マウス）
/// </summary>
export namespace input
{
    /// <summary>
    /// マウスクリック情報の管理
    /// </summary>
    export class MouseStatus final
    {
    public:
        /// <summary>
        /// インスタンスを返す
        /// </summary>
        /// <returns>自身のインスタンス</returns>
        static MouseStatus& Instance()
        {
            static MouseStatus instance;
            return instance;
        }

        /// <summary>
        /// クリック情報更新
        /// </summary>
        static void UpdateMouseState();

        /// <summary>
        /// クリック情報判定
        /// </summary>
        /// <param name="button">対象ボタン（例: 左=0, 右=1 など）</param>
        /// <param name="state">対象ステータス</param>
        /// <returns>一致:true | 不一致:false</returns>
        static bool CheckClick(const int button, const int state)
        {
            return Instance().CheckClickImpl(button, state);
        }

        /// <summary>
        /// マウス座標取得
        /// </summary>
        /// <returns>マウス座標</returns>
        static const Vector2i& GetMousePos()
        {
            return Instance().mousePos;
        }

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~MouseStatus() = default;

    private:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        MouseStatus();

        // 実装用メソッド
        void UpdateClickStateImpl();
        bool CheckClickImpl(const int button, const int state)
        {
            //押下中判定結果を返す
            return (clickData[button] & state);
        }

        Vector2i mousePos;                          // マウス座標
        std::unordered_map<int, int> clickData;     // クリック情報
    };
}
