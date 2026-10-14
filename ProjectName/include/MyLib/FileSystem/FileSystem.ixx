module;
#include <Windows.h>
#include <filesystem>
#include <fstream>

export module MyLib.File.FileSystem;

import <unordered_map>;
import <string>;

export import MyLib.File.JsonIO;
export import MyLib.File.CsvIO;
export import MyLib.File.MemMapFile;

namespace fs = std::filesystem;

/// <summary>
/// ファイル関連
/// </summary>
export namespace file
{
    /// <summary>
    /// ファイルシステムを管理する
    /// </summary>
    export class FileSystem final
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        FileSystem()
        {
            // 実行ファイルの場所の初期化
            InitExeDir();

            // リソースディレクトリの設定
            m_resourcesDir = m_exeDir / "resources";
            m_dataDir = m_resourcesDir / "data";
            m_assetsDir = m_resourcesDir / "assets";
            m_shaderDir = m_resourcesDir / "shader";

            // 仮想パスをマウント
            Mount("exe", m_exeDir);
            Mount("resources", m_resourcesDir);
            Mount("data", m_dataDir);
            Mount("assets", m_assetsDir);
            Mount("shader", m_shaderDir);
        }

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~FileSystem() = default;

        /// <summary>
        /// 実行ファイルの場所の初期化
        /// </summary>
        void InitExeDir()
        {
            char buffer[MAX_PATH];
            GetModuleFileNameA(nullptr, buffer, MAX_PATH);
            m_exeDir = std::filesystem::path(buffer).parent_path();
        }

        /// <summary>
        /// 実行ファイルの場所を取得
        /// </summary>
        /// <returns>実行ファイルが存在するディレクトリのパス</returns>
        fs::path GetExeDir() const { return m_exeDir; }

        /// <summary>
        /// resources の場所
        /// </summary>
        fs::path GetResourcesDir() const { return m_resourcesDir; }

        /// <summary>
        /// データファイルの場所を取得
        /// </summary>
        fs::path GetDataDir() const { return m_dataDir; }

        /// <summary>
        /// アセットディレクトリの場所を取得
        /// </summary>
        fs::path GetAssetsDir() const { return m_assetsDir; }

        /// <summary>
        /// シェーダーディレクトリの場所を取得
        /// </summary>
        fs::path GetShaderDir() const { return m_shaderDir; }

        /// <summary>
        /// 仮想パスのマウント
        /// </summary>
        void Mount(const std::string& scheme, const fs::path& root)
        {
            m_mounts[scheme] = fs::weakly_canonical(root);
        }

        /// <summary>
        /// 仮想パスの解決
        /// </summary>
        fs::path Resolve(std::string_view path) const
        {
            // スキームの位置を検索
            auto pos = path.find("://");
            if (pos == std::string_view::npos) {
                return fs::path(std::string(path));
            }

            // スキームとサブパスに分割
            std::string scheme{ path.substr(0, pos) };
            std::string sub{ path.substr(pos + 3) };
            
            // マウントポイントを探す
            auto it = m_mounts.find(scheme);
            if (it == m_mounts.end()) {
                return fs::path(std::string(path));
            }
            return it->second / sub;
        }

        /// <summary>
        /// ディレクトリの再帰コピー
        /// </summary>
        void CopyDir(fs::path from, fs::path to)
        {
            try
            {
                fs::copy(from, to,
                    fs::copy_options::recursive | fs::copy_options::overwrite_existing);
            }
            catch (const fs::filesystem_error& e)
            {
                // ログ出力（DxLibなら printfDx とかでもOK）
                throw e;
            }
        }

        JsonIO jsonIO;              // Json入出力
        CsvIO csvIO;                // CSV入出力
    private:
        fs::path m_exeDir;          // 実行ファイルディレクトリ
        fs::path m_resourcesDir;    // resources ディレクトリ
        fs::path m_dataDir;         // data ディレクトリ(resources 配下)
        fs::path m_assetsDir;       // assets ディレクトリ(resources 配下)
        fs::path m_shaderDir;       // shader ディレクトリ(resources 配下)
        std::unordered_map<std::string, fs::path> m_mounts; // マウントポイント
    };
}
