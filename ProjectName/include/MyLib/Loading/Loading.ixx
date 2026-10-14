module;
#include <functional>
#include <future>
#include <atomic>
#include <mutex>
#include <string>

export module MyLib.Loading;

import <memory>;
import <unordered_map>;
import <array>;
import <vector>;

/// <summary>
/// タスク関連
/// </summary>
export namespace task
{
    /// <summary>
    /// 読み込みレベル
    /// </summary>
    export enum Level :int16_t
    {
        DATA,   // データ読み込み
        INIT,   // 初期化
        GRAPH,  // グラフィック
        SOUND,  // サウンド
        FINAL,  // 最終調整
        END,    // レベル終了マーカー
    };

    /// <summary>
    /// ローディングに関する機能(非同期)
    /// </summary>
    export class Loading final
    {
    public:

        /// <summary>
        /// コンストラクタ
        /// </summary>
        Loading();

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Loading();

        /// <summary>
        /// ローディング開始
        /// </summary>
        void StartLoading();

        /// <summary>
        /// タスクの登録(登録のみで実行はしない。メインスレッド専用)
        /// </summary>
        /// <param name="level">読み込みレベル</param>
        /// <param name="task">追加するタスク(失敗時は例外を投げる)</param>
        void AddTask(Level level, std::function<void()> task);

        /// <summary>
        /// 登録済みタスクの実行開始(全タスク登録後に呼ぶ)
        /// </summary>
        void Run();

        /// <summary>
        /// 失敗の通知(スレッドセーフ)
        /// </summary>
        /// <param name="reason">失敗理由</param>
        void ReportFailure(const std::string& reason);

        /// <summary>
        /// メインスレッドで実行する処理を予約(ワーカーから呼んでよい。共有コンテナーの更新用)
        /// </summary>
        /// <param name="commit">メインスレッドで実行する処理</param>
        void PostToMain(std::function<void()> commit);

        /// <summary>
        /// 予約済みの処理を実行(メインスレッド専用)
        /// </summary>
        void PumpMain();

        /// <summary>
        /// ローディング中かどうか
        /// </summary>
        /// <returns>ローディング状況</returns>
        bool IsLoading() const
        {
            return m_isLoading.load();
        }

        /// <summary>
        /// 失敗したかどうか
        /// </summary>
        /// <returns>失敗していればtrue</returns>
        bool HasFailed() const
        {
            return m_failed.load();
        }

        /// <summary>
        /// 最初の失敗理由
        /// </summary>
        /// <returns>失敗理由</returns>
        std::string FailureReason() const;

        /// <summary>
        /// 現在の進捗度
        /// </summary>
        /// <returns>進捗度</returns>
        float NowProgress() const
        {
            const int total = m_totalTasks.load();
            const int done = m_finishTasks.load();
            return (total <= 0) ? 1.0f : float(done) / float(total);
        }

    private:
        /// <summary>
        /// レベル順にタスクを実行(ワーカー側)
        /// </summary>
        void Execute();

        /// <summary>
        /// 1タスクを例外安全に実行
        /// </summary>
        void RunTask(const std::function<void()>& task);

        std::array<std::vector<std::function<void()>>, (int)Level::END> m_tasks;   //レベル別タスク
        std::atomic<int> m_totalTasks{ 0 };     //総タスク数
        std::atomic<int> m_finishTasks{ 0 };    //終了タスク数(失敗・スキップ含む)
        std::atomic<bool> m_isLoading{ false }; //ローディング状況
        std::atomic<bool> m_failed{ false };    //失敗状況
        std::atomic<bool> m_cancel{ false };    //中断要求
        mutable std::mutex m_mutex;             //失敗理由の保護用
        std::string m_failReason;               //最初の失敗理由
        std::mutex m_mainMutex;                 //メインキュー保護用
        std::vector<std::function<void()>> m_mainQueue; //メインスレッド実行待ち
        std::future<void> m_runner;             //実行用スレッド
    };
}

