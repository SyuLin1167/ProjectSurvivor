export module Campaign.CampaignSession;
export import Campaign.CampaignProgress;
import <filesystem>;
import <vector>;

export namespace campaign
{
    /// <summary>不存在と、既存ファイルを保護すべき読込異常を区別する。</summary>
    enum class SaveStatus { Unloaded, Missing, Valid, Corrupt, Unsupported, IoError };

    /// <summary>アプリ寿命の攻略記録と保存境界。シーンや遠征オブジェクトを所有しない。</summary>
    class CampaignSession
    {
    public:
        /// <summary>ユーザーのLocalAppData内を保存先にする。ファイルはまだ変更しない。</summary>
        CampaignSession();
        /// <summary>保存先を明示する。テストも同じ保存処理を専用領域で使う。</summary>
        /// <param name="path">保存先JSONの絶対パス。</param>
        explicit CampaignSession(std::filesystem::path path);
        /// <summary>記録を読み直す。破損や未対応版を初期値で上書きしない。</summary>
        /// <param name="knownIds">現在登録されている依頼ID。</param>
        void Load(const std::vector<std::string>& knownIds);
        /// <summary>確認済みの新規開始。保存成功後にだけメモリ上の記録を初期化する。</summary>
        /// <returns>初期記録を保存できた場合true。失敗時は既存記録を保持する。</returns>
        bool StartNew();
        /// <summary>最終区間成功を記録して保存する。保存失敗時も実績をメモリに残す。</summary>
        /// <param name="id">成功した検証済み依頼ID。</param>
        void RecordClear(const std::string& id);
        /// <summary>未保存実績を書き出す。結果画面・タイトルから再試行できる。</summary>
        /// <returns>保存成功または保存不要ならtrue。</returns>
        bool Save();
        /// <summary>読込状態を返す。</summary>
        /// <returns>不存在・正常・異常の区分。</returns>
        SaveStatus Status() const { return m_status; }
        /// <summary>出発できる攻略記録を持つかを返す。</summary>
        /// <returns>正常な記録がある場合true。</returns>
        bool Ready() const { return m_status == SaveStatus::Valid; }
        /// <summary>保存されていない実績があるかを返す。</summary>
        /// <returns>保存失敗後の再試行が必要ならtrue。</returns>
        bool Dirty() const { return m_dirty; }
        /// <summary>攻略実績を読み取る。</summary>
        /// <returns>セッションが所有する実績。変更は保存境界を通す。</returns>
        const CampaignProgress& Progress() const { return m_progress; }
        /// <summary>ユーザー向けの日本語状態文を返す。</summary>
        /// <returns>保存状態と、異常時の対応案内。</returns>
        std::string Message() const;
        /// <summary>保存先を確認する。</summary>
        /// <returns>実行ファイル・resourcesと独立した保存パス。</returns>
        const std::filesystem::path& Path() const { return m_path; }
    private:
        /// <summary>同じディレクトリの一時ファイルをflushしてから置換する。</summary>
        /// <param name="progress">保存する値。</param>
        /// <returns>置換まで成功した場合true。</returns>
        bool Write(const CampaignProgress& progress);
        std::filesystem::path m_path; // ユーザー領域のcampaign.json。
        CampaignProgress m_progress;
        SaveStatus m_status = SaveStatus::Unloaded;
        bool m_dirty = false; // ファイルに反映できていない成功実績。
        bool m_writeFailed = false; // 新規開始失敗も既存データと分けて通知する。
    };
}
