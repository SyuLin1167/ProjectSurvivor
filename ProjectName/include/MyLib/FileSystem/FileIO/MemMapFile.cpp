module;
#include <Windows.h>

module MyLib.File.MemMapFile;

namespace file
{
    MemMapFile::MemMapFile()
        : m_fileHandle(INVALID_HANDLE_VALUE)
        , m_mapHandle()
        , m_ptr()
    {
        //処理なし
    }

    MemMapFile::MemMapFile(MemMapFile&& other) noexcept
        : m_fileHandle(other.m_fileHandle)
        , m_mapHandle(other.m_mapHandle)
        , m_ptr(other.m_ptr)
    {
        other.m_fileHandle = INVALID_HANDLE_VALUE;
        other.m_mapHandle = nullptr;
        other.m_ptr = nullptr;
    }

    MemMapFile& MemMapFile::operator=(MemMapFile&& other) noexcept
    {
        if (this != &other)
        {
            Close();
            m_fileHandle = other.m_fileHandle;
            m_mapHandle = other.m_mapHandle;
            m_ptr = other.m_ptr;
            other.m_fileHandle = INVALID_HANDLE_VALUE;
            other.m_mapHandle = nullptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    MemMapFile::~MemMapFile()
    {
        Close();
    }

    bool MemMapFile::Open(const char* fileName)
    {
        Close();

        //ファイルハンドルの作成
        m_fileHandle = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (m_fileHandle == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        // サイズ0のファイルは MapViewできないので特別扱い
        if (GetFileSize() == 0) {
            m_mapHandle = nullptr;
            m_ptr = nullptr;
            return true;
        }

        //ハンドルのマッピング
        m_mapHandle = CreateFileMapping(m_fileHandle, 0, PAGE_READONLY, 0, 0, 0);
        if (!m_mapHandle)
        {
            CloseHandle(m_fileHandle);
            m_fileHandle = INVALID_HANDLE_VALUE;
            return false;
        }

        //マッピングデータをポインタへ格納
        m_ptr = static_cast<char*>(MapViewOfFile(m_mapHandle, FILE_MAP_READ, 0, 0, 0));
        return true;
    }

    size_t MemMapFile::GetFileSize() const noexcept
    {
        if (m_fileHandle != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER li{};
            if (GetFileSizeEx(m_fileHandle, &li))
            {
                return static_cast<size_t>(li.QuadPart);
            }
        }
        return 0;
    }

    void MemMapFile::Close()
    {
        // 確保したリソースの解放
        if (m_ptr)
        {
            UnmapViewOfFile(m_ptr);
            m_ptr = nullptr;
        }

        // マッピングハンドルの解放
        if (m_mapHandle)
        {
            CloseHandle(m_mapHandle);
            m_mapHandle = nullptr;
        }

        // ファイルハンドルの解放
        if (m_fileHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_fileHandle);
            m_fileHandle = INVALID_HANDLE_VALUE;
        }
    }
}
