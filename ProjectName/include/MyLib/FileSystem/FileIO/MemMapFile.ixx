module;
#include <Windows.h>
#include <filesystem>

export module MyLib.File.MemMapFile;

/// <summary>
/// ファイル関連
/// </summary>
export namespace file
{
    /// <summary>
    /// メモリマップファイルに関する処理を行う
    /// </summary>
    export class MemMapFile
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        MemMapFile();

        // コピー不可
        MemMapFile(const MemMapFile&) = delete;
        MemMapFile& operator=(const MemMapFile&) = delete;

        // ムーブ可
        MemMapFile(MemMapFile&&) noexcept;
        MemMapFile& operator=(MemMapFile&&) noexcept;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~MemMapFile();

        /// <summary>
        /// ファイルを開く(読み込み)
        /// </summary>
        /// <param name="fileName">ファイルパス</param>
        /// <returns>開けたかどうか</returns>
        bool Open(const char* fileName);

        /// <summary>
        /// ファイルポインタの取得
        /// </summary>
        /// <param name="ptr">ポインタ</param>
        char* GetPtr() noexcept { return m_ptr; };
        const char* GetPtr() const noexcept { return m_ptr; };

        /// <summary>
        /// ファイルサイズの取得
        /// </summary>
        /// <returns>ファイルサイズ</returns>
        size_t GetFileSize() const noexcept;

        /// <summary>
        /// ファイルを閉じる
        /// </summary>
        void Close();
    private:
        HANDLE m_fileHandle;  //ファイルハンドル
        HANDLE m_mapHandle;   //マッピングハンドル
        char* m_ptr;      //ポインタ
    };
}
