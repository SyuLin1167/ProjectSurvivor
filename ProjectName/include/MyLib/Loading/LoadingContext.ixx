export module MyLib.Loading.LoadingContext;

export import MyLib.Loading;

/// <summary>
/// タスク関連
/// </summary>
export namespace task
{
    /// <summary>
    /// Loading用スコープコンテキスト
    /// </summary>
    export class LoadingContext final
    {
    public:
        /// <summary>
        /// Loadingクラスの確保
        /// </summary>
        /// <param name="loading">確保するLoading</param>
        static void Set(Loading* loading)
        {
            m_current = loading;
        }

        /// <summary>
        /// Loadingの取得
        /// </summary>
        /// <returns>確保中のLoading</returns>
        static Loading* Get()
        {
            return m_current;
        }

        /// <summary>
        /// Loadingの解放
        /// </summary>
        static void Release()
        {
            delete m_current;
            m_current = nullptr;
        }
    private:
        static inline thread_local Loading* m_current = nullptr;  //確保中のLoading
    };
}
