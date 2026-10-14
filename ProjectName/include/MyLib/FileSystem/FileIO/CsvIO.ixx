module;
#include <future>

export module MyLib.File.CsvIO;

import <string>;
import <filesystem>;
import <vector>;
import <charconv>;
import <memory>;

import MyLib.Loading.LoadingContext;
import MyLib.File.MemMapFile;

/// <summary>
/// ファイル関連
/// </summary>
export namespace file
{
    /// <summary>
    /// Csvファイルの読み書きを行う
    /// </summary>
    export class CsvIO final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        CsvIO() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~CsvIO() = default;

        /// <summary>
        /// Csvファイルの読み込み
        /// </summary>
        /// <param name="fileName">ファイル名</param>
        /// <returns>メモリマップファイル</returns>
        file::MemMapFile Load(std::filesystem::path fileName)
        {
            // 拡張子が無ければ付与する
            if (fileName.extension() != ".csv") {
                fileName += ".csv";
            }

            // メモリマップで開く
            file::MemMapFile mmf;
            mmf.Open(fileName.string().c_str());
            return mmf;
        }

        /// <summary>
        /// Csvファイルの同期読み込み
        /// </summary>
        /// <param name="fileName">ファイル名</param>
        /// <returns>メモリマップファイルの非同期結果</returns>
        std::future<file::MemMapFile> LoadAsync(std::filesystem::path fileName)
        {
            // 非同期タスクを作成
            auto task = std::make_shared<std::packaged_task<file::MemMapFile()>>(
                [this, fileName]() {
                    return Load(fileName);
                });

            // ローディングコンテキストがあればタスクを登録、なければ即時実行
            auto fut = task->get_future();
            if (task::LoadingContext::Get())
            {
                task::LoadingContext::Get()->AddTask(task::DATA, [task = std::move(task)]() {
                    (*task)();
                });
            }
            else
            {
                (*task)();
            }
            return fut;
        }

        /// <summary>
        /// Csvファイルのパース
        /// </summary>
        /// <param name="mmf">メモリマップファイル</param>
        /// <returns>パース結果</returns>
        std::vector<std::string_view> Parse(const file::MemMapFile& mmf)
        {
            // メモリマップからデータ取得
            const char* ptr = mmf.GetPtr();
            size_t size = mmf.GetFileSize();
            std::vector<std::string_view> data;
            size_t start = 0;

            // カンマ・改行で分割
            for (size_t i = 0; i < size; ++i) {
                if (ptr[i] == ',' || ptr[i] == '\n' || ptr[i] == '\r')
                {
                    if (i > start) {
                        data.emplace_back(&ptr[start], i - start);
                    }
                    start = i + 1;
                }
            }

            // 最後のデータを追加
            if (start < size) {
                data.emplace_back(&ptr[start], size - start);
            }

            return data;
        }

        /// <summary>
        /// Csvデータから配列を作成
        /// </summary>
        /// <typeparam name="T">配列の型</typeparam>
        /// <param name="data">Csvデータ</param>
        /// <returns>作成した配列</returns>
        template<typename T>
        std::vector<T> CreateArray(const std::vector<std::string_view>& data)
        {
            // 配列の準備
            std::vector<T> result;
            result.reserve(data.size());

            // 文字列を型に変換して配列に追加
            for (const auto& item : data)
            {
                // 変換用変数
                T value{};
                std::errc ec{};
                const char* end = nullptr;

                // 変換
                if constexpr (std::is_floating_point_v<T>) {
                    auto [p, e] = std::from_chars(item.data(), item.data() + item.size(), value, std::chars_format::general);
                    end = p;
                    ec = e;
                }
                else {
                    auto [p, e] = std::from_chars(item.data(), item.data() + item.size(), value);
                    end = p;
                    ec = e;
                }

                // 変換成功時のみ追加
                if (ec == std::errc())
                {
                    result.emplace_back(value);
                }
                else
                {
                    throw std::runtime_error("Failed to convert CSV data to target type.");
                }
            }
            return result;
        }

        /// <summary>
        /// Csvファイルから配列を作成
        /// </summary>
        /// <typeparam name="T">配列の型</typeparam>
        /// <param name="fileName">ファイル名</param>
        /// <returns>作成した配列</returns>
        template<typename T>
        inline std::vector<T> CreateArray(std::filesystem::path fileName)
        {
            auto mmf = Load(fileName);
            auto data = Parse(mmf);
            return CreateArray<T>(data);
        }

        /// <summary>
        /// Csvファイルから配列を非同期作成
        /// </summary>
        /// <typeparam name="T">配列の型</typeparam>
        /// <param name="fileName">ファイル名</param>
        /// <returns>作成した配列の非同期結果</returns>
        template<typename T>
        void CreateArrayAsync(std::filesystem::path fileName, std::vector<T>& result)
        {
            // 非同期タスクを作成
            auto task = std::make_shared<std::packaged_task<std::vector<T>()>>(
                [this, fileName]() {
                    return CreateArray<T>(fileName);
                });

            // ローディングコンテキストがあればタスクを登録、なければ即時実行
            if (task::LoadingContext::Get())
            {
                task::LoadingContext::Get()->AddTask(task::DATA, [&result, task = std::move(task)]() mutable {
                    (*task)();
                    result = task->get_future().get();
                });
            }
            else
            {
                (*task)();
                result = task->get_future().get();
            }
        }

        /// <summary>
        /// Csvファイルから配列を非同期作成
        /// </summary>
        /// <typeparam name="T">配列の型</typeparam>
        /// <param name="fileName">ファイル名</param>
        /// <returns>作成した配列の非同期結果</returns>
        template<typename T>
        std::future<std::vector<T>> CreateArrayAsync(std::filesystem::path fileName)
        {
            // 非同期タスクを作成
            auto task = std::make_shared<std::packaged_task<std::vector<T>()>>(
                [this, fileName]() {
                    return CreateArray<T>(fileName);
                });
            // ローディングコンテキストがあればタスクを登録、なければ即時実行
            auto fut = task->get_future();
            if (task::LoadingContext::Get())
            {
                task::LoadingContext::Get()->AddTask(task::DATA, [task = std::move(task)]() {
                    (*task)();
                });
            }
            else
            {
                (*task)();
            }
            return fut;
        }
    };
}
