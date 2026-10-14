export module GameSystem.Library;

/// <summary>
/// システム関連
/// </summary>
export namespace gameSystem
{
    /// <summary>
    /// ライブラリの管理
    /// </summary>
    export class Library final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        Library();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Library();

        /// <summary>
        /// 初期化成功判定
        /// </summary>
        /// <returns>成功:true|失敗:false</returns>
        const bool InitSuccess() const
        {
            return m_success;
        }

    private:
        bool m_success;        //成功状態
    };
}

