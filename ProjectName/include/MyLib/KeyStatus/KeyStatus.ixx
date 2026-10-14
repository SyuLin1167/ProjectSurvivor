export module MyLib.KeyStatus;

import <unordered_map>;

export import MyLib.KeyStatus.KeyTag;

export constexpr int8_t ON_RELEASE = 0x0001;      //未入力時
export constexpr int8_t RELEASING = 0x0002;       //未入力中
export constexpr int8_t ON_PRESS = 0x0004;        //入力時
export constexpr int8_t PRESSING = 0x0008;        //入力中

/// <summary>
/// 入力関連
/// </summary>
export namespace input
{
    /// <summary>
    /// 入力情報の管理
    /// </summary>
    export class KeyStatus final
    {
    public:
        /// <summary>
        /// コピー禁止令
        /// </summary>
        KeyStatus(const KeyStatus&) = delete;
        KeyStatus(KeyStatus&&) = delete;

        /// <summary>
        /// 代入禁止令
        /// </summary>
        KeyStatus& operator=(const KeyStatus&) = delete;
        KeyStatus& operator=(KeyStatus&&) = delete;

        /// <summary>
        /// キー情報更新
        /// </summary>
        static void UpdateKeyState()
        {
            Instance().UpdateKeyStateImpl();
        }

        /// <summary>
        /// キー情報判定
        /// </summary>
        /// <param name="key">対象キー</param>
        /// <param name="state">対象ステータス</param>
        /// <returns>一致:true|不一致:false</returns>
        [[nodiscard]] static bool CheckKey(const int key, const int state)
        {
            return Instance().CheckKeyImpl(key, state);
        }

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~KeyStatus() = default;

    private:
        /// <summary>
        /// インスタンスを返す
        /// </summary>
        /// <returns>自身のインスタンス</returns>
        static KeyStatus& Instance()
        {
            static KeyStatus instance;
            return instance;
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        KeyStatus();

        // 実装用メソッド
        void UpdateKeyStateImpl();
        bool CheckKeyImpl(const int key, const int state)
        {
            //押下中判定結果を返す
            return (keyData[key] & state);
        }

        std::unordered_map<int, int> keyData;     //キー情報
    };

}