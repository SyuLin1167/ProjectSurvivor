export module GameSystem.Window;

import <memory>;

import MyLib.Math.Vector2;

using namespace math;

/// <summary>
/// システム関連
/// </summary>
export namespace gameSystem
{
    /// <summary>
    /// ウィンドウの管理
    /// </summary>
    export class Window final
    {
        struct Info;

    public:
        /// <summary>
        /// コピー禁止令
        /// </summary>
        Window(const Window&) = delete;
        Window(Window&&) = delete;

        /// <summary>
        /// 代入禁止令
        /// </summary>
        Window& operator=(const Window&) = delete;
        Window& operator=(Window&&) = delete;

        /// <summary>
        /// 初期化処理
        /// </summary>
        static void Init()
        {
            Instance();
        }

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Window() = default;

        /// <summary>
        /// ウィンドウ情報取得
        /// </summary>
        /// <returns>ウィンドウ情報</returns>
        static const Info* GetWindowData()
        {
            return Instance().m_winInfo.get();
        }

    private:
        /// <summary>
        /// インスタンスを返す
        /// </summary>
        /// <returns>自身のインスタンス</returns>
        static Window& Instance()
        {
            static Window instance;
            return instance;
        }

        /// <summary>
        /// コンストラクタ
        /// </summary>
        Window();

        /// <summary>
        /// ウィンドウ情報
        /// </summary>
        const struct Info
        {
            const Vector2<uint32_t> SIZE{ 1920, 1080 };    //ウィンドウサイズ
            const int COLOR_BIT = 32;    //カラービット
        };

        std::unique_ptr<Info> m_winInfo;        //ウィンドウ情報
    };
}

