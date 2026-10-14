module;
#include <DxLib.h>
#include <format>
#include <future>
#include <thread>
#include <filesystem>

module Asset.Graph;

import MyLib.Loading.LoadingContext;
import AppContext;

namespace asset
{
    Graph::Graph()
        :handles()
    {
        //処理なし
    }

    Graph::~Graph()
    {
        //確保した画像の解放
        for (auto& handle : handles)
        {
            DeleteGraph(handle.second);
        }
    }

    void Graph::CreateHandle(std::string handleName, std::string graphName)
    {
        // 以前のハンドルを削除
        DeleteHandle(handleName);

        // グラフィック読み込み
        auto fpath = gameSystem::AppCtx::FileSystem().Resolve(std::format("assets://{}", graphName));
        int handle = LoadGraph(fpath.string().c_str());

        handles[handleName] = handle;
    }

    void Graph::CreateHandleAsync(std::string handleName, std::string graphName)
    {
        // 以前のハンドルを削除
        DeleteHandle(handleName);

        // グラフィック読み込み
        auto fpath = gameSystem::AppCtx::FileSystem().Resolve(std::format("assets://{}", graphName));
        int handle = LoadGraph(fpath.string().c_str());

        // 読み込み開始に失敗した場合は待機せず終了
        if (handle == -1)
        {
            ErrorLogFmtAdd("Graph load start failed: %s", graphName.c_str());
            if (task::LoadingContext::Get())
            {
                task::LoadingContext::Get()->ReportFailure("Graph load start failed: " + graphName);
            }
            return;
        }

        // タスクを作成
        auto* loading = task::LoadingContext::Get();
        auto task = [this, loading, handle, handleName, graphName]()
            {
                // 読み込み完了を待機(失敗時は登録しない)
                if (!WaitAsyncLoad(handle))
                {
                    ErrorLogFmtAdd("Graph async load failed: %s", graphName.c_str());
                    DeleteGraph(handle);
                    throw std::runtime_error("Graph async load failed: " + graphName);
                }

                // 共有コンテナーへの登録はメインスレッドで行う
                if (loading)
                {
                    loading->PostToMain([this, handleName, handle]() { handles[handleName] = handle; });
                }
                else
                {
                    handles[handleName] = handle;
                }
            };

        // 非同期読み込み中ならタスクに登録
        if (task::LoadingContext::Get())
        {
            task::LoadingContext::Get()->AddTask(task::GRAPH, task);
        }
        else
        {
            task();
        }
    }

    void Graph::DeleteHandle(std::string name)
    {
        //対象のハンドルを見つけて削除
        auto it = handles.find(name);
        if (it != handles.end())
        {
            DeleteGraph(it->second);
            handles.erase(it);
        }
    }
}
