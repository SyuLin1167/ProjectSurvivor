export module MyLib.ClickStatus;

import <unordered_map>;

export constexpr int8_t CLICK_ON_RELEASE = 0x0001;   // 未入力時
export constexpr int8_t CLICK_RELEASING  = 0x0002;   // 未入力中
export constexpr int8_t CLICK_ON_PRESS   = 0x0004;   // 入力時
export constexpr int8_t CLICK_PRESSING   = 0x0008;   // 入力中

/// <summary>
/// 入力関連（クリック）
/// </summary>
export namespace input
{
    /// <summary>
    /// クリック情報の管理
    /// </summary>
    export class ClickStatus final
    {
    public:
        /// <summary>
        /// インスタンスを返す
        /// </summary>
        /// <returns>自身のインスタンス</returns>
        static ClickStatus& Instance()
        {
            static ClickStatus instance;
            return instance;
        }

        /// <summary>
        /// クリック情報更新
        /// </summary>
        inline void UpdateClickState()
        {
            Instance().UpdateClickStateImpl();
        }

        /// <summary>
        /// クリック情報判定
        /// </summary>
        /// <param name="button">対象ボタン（例: 左=0, 右=1 など）</param>
        /// <param name="state">対象ステータス</param>
        /// <returns>一致:true | 不一致:false</returns>
        static bool CheckClick(const int& button, const int& state)
        {
            return Instance().CheckClickImpl(button, state);
        }

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~ClickStatus() = default;

    private:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        ClickStatus();

        // 実装用メソッド
        void UpdateClickStateImpl();
        bool CheckClickImpl(const int button, const int state)
        {
            //押下中判定結果を返す
            return (clickData[button] & state);
        }

        std::unordered_map<int, int> clickData;          // クリック情報
    };
}
