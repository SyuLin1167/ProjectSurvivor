module;
#include <DxLib.h>
#include <format>
#include <future>
#include <thread>
#include <filesystem>

module Asset.DivisionGraph;

import MyLib.Loading.LoadingContext;
import AppContext;

namespace asset
{
    DivisionGraph::~DivisionGraph()
    {
        //確保した画像の解放
        for (auto& info : m_handles)
        {
            for (int i = 0; i < info.second.handle.size(); i++)
            {
                DeleteGraph(info.second.handle[i]);
            }
            info.second.handle.clear();
        }
    }

    void DivisionGraph::CreateHandle(std::string handleName, std::string graphName)
    {
        // 以前のハンドルを削除
        DeleteHandle(handleName);

        // 画像読み込み
        auto fpath = gameSystem::AppCtx::FileSystem().Resolve(std::format("assets://{}", graphName));
        int img = LoadGraph(fpath.string().c_str());

        // 読み込んだ画像から分割読み込みに必要な情報を保存
        int imgW, imgH;
        GetGraphSize(img, &imgW, &imgH);
        DeleteGraph(img);
        int col = imgW / m_divW;
        int row = imgH / m_divH;
        int total = col * row;
        m_handles[handleName].handle.resize(total);

        //取得した情報から画像分割読み込み
        LoadDivGraph(fpath.string().c_str(), total, col, row, m_divW, m_divH, m_handles[handleName].handle.data());
    }

    void DivisionGraph::CreateHandleAsync(std::string handleName, std::string graphName)
    {
        // 以前のハンドルを削除
        DeleteHandle(handleName);

        // 画像を読み込まずにファイルから画像サイズを取得(非同期フラグの切り替えは不要)
        auto fpath = gameSystem::AppCtx::FileSystem().Resolve(std::format("assets://{}", graphName));
        int imgW = 0, imgH = 0;

        // 存在しないファイルをDxLibへ渡すと内部でクラッシュするため事前確認
        std::error_code ec;
        if (!std::filesystem::exists(fpath, ec))
        {
            ErrorLogFmtAdd("DivisionGraph file not found: %s", graphName.c_str());
            if (task::LoadingContext::Get()) { task::LoadingContext::Get()->ReportFailure("DivisionGraph file not found: " + graphName); }
            return;
        }
        if (GetImageSize_File(fpath.string().c_str(), &imgW, &imgH) == -1)
        {
            ErrorLogFmtAdd("DivisionGraph size acquisition failed: %s", graphName.c_str());
            if (task::LoadingContext::Get()) { task::LoadingContext::Get()->ReportFailure("DivisionGraph size acquisition failed: " + graphName); }
            return;
        }

        // 取得した情報から分割数を算出
        int col = imgW / m_divW;
        int row = imgH / m_divH;
        int total = col * row;
        if (total <= 0)
        {
            ErrorLogFmtAdd("DivisionGraph invalid division: %s", graphName.c_str());
            if (task::LoadingContext::Get()) { task::LoadingContext::Get()->ReportFailure("DivisionGraph invalid division: " + graphName); }
            return;
        }
        m_handles[handleName].handle.resize(total);

        // 取得した情報から画像分割読み込み
        if (LoadDivGraph(fpath.string().c_str(), total, col, row, m_divW, m_divH, m_handles[handleName].handle.data()) == -1)
        {
            ErrorLogFmtAdd("DivisionGraph load start failed: %s", graphName.c_str());
            m_handles.erase(handleName);
            if (task::LoadingContext::Get()) { task::LoadingContext::Get()->ReportFailure("DivisionGraph load start failed: " + graphName); }
            return;
        }

        // ワーカーは共有コンテナーに触れず、ハンドルの値コピーだけを待つ
        std::vector<int> handleCopy = m_handles[handleName].handle;
        auto task = [handleCopy, handleName, graphName]()
            {
                // 非同期読み込み完了待ち(分割分のハンドルの完了状況を見る)
                for (int h : handleCopy)
                {
                    if (!WaitAsyncLoad(h))
                    {
                        ErrorLogFmtAdd("DivisionGraph async load failed: %s", graphName.c_str());
                        throw std::runtime_error("DivisionGraph async load failed: " + graphName);
                    }
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

    void DivisionGraph::DeleteHandle(std::string name)
    {
        //ハンドルを検索して削除
        auto it = m_handles.find(name);
        if (it == m_handles.end())
        {
            return;
        }

        for (int i = 0; i < it->second.handle.size(); ++i)
        {
            DeleteGraph(it->second.handle[i]);
        }

        m_handles.erase(it);
    }
}