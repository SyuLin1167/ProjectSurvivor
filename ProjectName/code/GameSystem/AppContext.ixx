export module AppContext;

import <optional>;

export import MyLib.File.FileSystem;
export import Asset.AssetManager;
export import UI.UIManager;
export import Campaign.CampaignSession;

/// <summary>
/// オブジェクト関連
/// </summary>
export namespace gameSystem
{
    /// <summary>
    /// 共有コンテキスト
    /// </summary>
    export struct AppContext final
    {
        /// <summary>
        /// アクティブ化
        /// </summary>
        static void Activate()
        {
            fileSystem.emplace();
            assetMgr.emplace();
            uiMgr.emplace();
            m_campaign.emplace();
        }

        /// <summary>
        /// 非アクティブ化
        /// </summary>
        static void Deactivate()
        {
            m_campaign.reset();
            fileSystem.reset();
            assetMgr.reset();
            uiMgr.reset();
        }

        /// <summary>
        /// ファイルシステムのインスタンスを返す
        /// </summary>
        /// <returns>ファイルシステムのインスタンス</returns>
        static file::FileSystem& FileSystem()
        {
            return *fileSystem;
        }

        /// <summary>
        /// アセット管理のインスタンスを返す
        /// </summary>
        /// <returns>アセット管理のインスタンス</returns>
        static asset::AssetManager& AssetMgr()
        {
            return *assetMgr;
        }

        /// <summary>
        /// UI管理のインスタンスを返す
        /// </summary>
        /// <returns>UI管理のインスタンス</returns>
        static ui::UIManager& UIMgr()
        {
            return *uiMgr;
        }

        /// <summary>遠征を跨いで保持する攻略記録と保存境界を返す。</summary>
        /// <returns>アプリが所有するセッション。シーンは所有せず、Retryでも作り直さない。</returns>
        static campaign::CampaignSession& Campaign() { return *m_campaign; }

    private:
        inline static std::optional<campaign::CampaignSession> m_campaign; // アプリ終了まで保持する攻略記録。
        inline static std::optional<file::FileSystem> fileSystem{};     // ファイルシステムのインスタンス
        inline static std::optional<asset::AssetManager> assetMgr{};    // アセット管理のインスタンス
        inline static std::optional<ui::UIManager> uiMgr{};             // UI管理のインスタンス
    };

    using AppCtx = AppContext;
}
