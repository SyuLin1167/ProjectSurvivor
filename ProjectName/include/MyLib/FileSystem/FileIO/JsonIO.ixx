module;
#include <fstream>
#include <future>

export module MyLib.File.JsonIO;

export import <json.hpp>;
import <memory>;

import MyLib.Loading.LoadingContext;

export using json = nlohmann::json;

/// <summary>
/// ファイル関連
/// </summary>
export namespace file
{
    /// <summary>
    /// Jsonファイルの読み書きを行う
    /// </summary>
    export class JsonIO final
    {
    public:
        /// <summary>
        /// JSONファイル読み込み
        /// </summary>
        /// <param name="fileName">ファイルパス</param>
        /// <returns>読み込んだデータ</returns>
        json Load(std::filesystem::path fileName)
        {
            // 拡張子が無ければ付与する
            if (fileName.extension() != ".json") {
                fileName += ".json";
            }

            // 指定ファイルからデータ読み込み
            std::ifstream ifs(fileName.string());
            if (!ifs.is_open())
            {
                throw std::runtime_error("Failed open file.");
            }
            json fileData;
            ifs >> fileData;
            ifs.close();
            return fileData;
        }

        /// <summary>
        /// JSONファイル非同期読み込み
        /// </summary>
        /// <param name="fileName">ファイルパス</param>
        /// <returns>読み込んだデータのfuture</returns>
        std::future<json> LoadAsync(std::filesystem::path fileName)
        {
            auto task = std::make_shared<std::packaged_task<json()>>([this, fileName]() {
                return Load(fileName);
                });
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
        /// JSONファイル読み込み
        /// </summary>
        /// <param name="fileName">ファイルパス</param>
        /// <param name="data">読み込んだデータの格納先</param>
        void Load(std::filesystem::path fileName, json& data)
        {
            // 拡張子が無ければ付与する
            if (fileName.extension() != ".json") {
                fileName += ".json";
            }

            std::ifstream ifs(fileName.string());
            if (!ifs.is_open())
            {
                throw std::runtime_error("Failed open file.");
            }
            json fileData;
            ifs >> fileData;
            ifs.close();
            data = fileData;
        }

        /// <summary>
        /// JSONファイル書き込み
        /// </summary>
        /// <param name="fileName">ファイルパス</param>
        /// <param name="data">書き込むデータ</param>
        void Write(std::filesystem::path fileName, json data)
        {
            // 親ディレクトリを抽出
            const auto parentDir = fileName.parent_path();

            // ディレクトリが存在しなければ再帰的に作成
            if (!std::filesystem::exists(parentDir)) {
                std::filesystem::create_directories(parentDir);
            }

            // 拡張子が無ければ付与する
            if (fileName.extension() != ".json") {
                fileName.string() += ".json";
            }

            // 指定ファイルにデータ書き込み
            std::ofstream ofs(fileName.string());
            if (!ofs.is_open())
            {
                throw std::runtime_error("Failed open file.");
            }
            ofs << data.dump(4);
            ofs.close();
        }
    };
}