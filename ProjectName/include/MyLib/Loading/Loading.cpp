
module;
#include <thread>
#include <future>
#include <chrono>
#include <exception>
#include <mutex>
#include <string>

module MyLib.Loading;

namespace task
{
	Loading::Loading()
	{
		// 処理なし
	}

	Loading::~Loading()
	{
		// 未実行タスクを中断し、実行中スレッドの終了を待つ
		m_cancel.store(true);
		if (m_runner.valid())
		{
			m_runner.wait();
		}
	}

	void Loading::StartLoading()
	{
		// ロード開始前に状態をリセット
		m_cancel.store(false);
		m_failed.store(false);
		m_totalTasks.store(0);
		m_finishTasks.store(0);
		m_isLoading.store(true);
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_failReason.clear();
		}
		for (auto& level : m_tasks)
		{
			level.clear();
		}
	}

	void Loading::AddTask(Level level, std::function<void()> task)
	{
		// 登録のみ(Run呼び出しまで実行しない)
		if (level < 0 || level >= Level::END || !task)
		{
			return;
		}
		m_tasks[(int)level].emplace_back(std::move(task));
		m_totalTasks.fetch_add(1);
	}

	void Loading::Run()
	{
		// すでに実行中なら何もしない
		if (m_runner.valid())
		{
			return;
		}

		// 実行用スレッドを作成
		m_runner = std::async(std::launch::async, [this]() { Execute(); });
	}

	void Loading::ReportFailure(const std::string& reason)
	{
		// 失敗を報告
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (!m_failed.load())
			{
				m_failReason = reason;
			}
		}
		m_failed.store(true);
	}

	std::string Loading::FailureReason() const
	{
		// 失敗理由を取得
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_failReason;
	}

	void Loading::PostToMain(std::function<void()> commit)
	{
		// メインスレッドで実行する処理を予約
		if (!commit)
		{
			return;
		}
		std::lock_guard<std::mutex> lock(m_mainMutex);
		m_mainQueue.emplace_back(std::move(commit));
	}

	void Loading::PumpMain()
	{
		// ロック内で取り出し、実行はロック外で行う
		std::vector<std::function<void()>> commits;
		{
			std::lock_guard<std::mutex> lock(m_mainMutex);
			commits.swap(m_mainQueue);
		}

		// メインスレッドで実行する処理を実行
		for (auto& commit : commits)
		{
			commit();
		}
	}

	void Loading::RunTask(const std::function<void()>& task)
	{
		// タスク実行中に例外が発生した場合は失敗として報告
		try
		{
			task();
		}
		catch (const std::exception& e)
		{
			ReportFailure(e.what());
		}
		catch (...)
		{
			ReportFailure("unknown exception");
		}
	}

	void Loading::Execute()
	{
		// レベル順に実行(同一レベルは並列、レベル間は完了待ち)
		for (auto& level : m_tasks)
		{
			std::vector<std::future<void>> running;
			running.reserve(level.size());

			// 同一レベルのタスクを並列実行

			for (auto& t : level)
			{
				if (m_failed.load() || m_cancel.load())
				{
					m_finishTasks.fetch_add(1);
					continue;
				}
				running.emplace_back(std::async(std::launch::async, [this, fn = std::move(t)]() {
					RunTask(fn);
					m_finishTasks.fetch_add(1);
					}));
			}

			// 同一レベルのタスクがすべて完了するまで待機
			for (auto& f : running)
			{
				f.wait();
			}
			level.clear();
		}

		m_isLoading.store(false);
	}
}
