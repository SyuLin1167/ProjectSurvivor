module;
#include <Windows.h>
export module Quest.QuestText;
import <string>;
import <stdexcept>;
export namespace quest
{
    /// <summary>UTF-8の依頼テキストを既存DxLib UIのコードページへ変換する。</summary>
    /// <param name="text">JSONから取得したUTF-8文字列。</param>
    /// <returns>既存UIへ渡すマルチバイト文字列。</returns>
    inline std::string DisplayText(const std::string& text)
    {
        if (text.empty()) return {};
        const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (size == 0) throw std::runtime_error("Quest text is not UTF-8");
        std::wstring wide(size, L' ');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), size);
        const int bytes = WideCharToMultiByte(CP_ACP, 0, wide.data(), size, nullptr, 0, nullptr, nullptr);
        std::string result(bytes, ' ');
        WideCharToMultiByte(CP_ACP, 0, wide.data(), size, result.data(), bytes, nullptr, nullptr);
        return result;
    }
}
