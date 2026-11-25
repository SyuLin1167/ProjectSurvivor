module;
#include <Windows.h>
#include <ShlObj.h>
#include <fstream>
#include <json.hpp>
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Ole32.lib")
module Campaign.CampaignSession;
import <algorithm>;
import MyLib.File.JsonValue;

namespace campaign
{
    CampaignSession::CampaignSession()
    {
        // 書込権限が保証されないexe隣を避け、日本語ユーザー名もワイドパスで扱う。
        PWSTR directory = nullptr;
        const auto result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &directory);
        if (SUCCEEDED(result)) m_path = std::filesystem::path(directory) / "ProjectSurvivor" / "campaign.json";
        CoTaskMemFree(directory);
    }

    CampaignSession::CampaignSession(std::filesystem::path path) : m_path(std::move(path)) {}

    void CampaignSession::Load(const std::vector<std::string>& knownIds)
    {
        // 保存失敗した直後の実績を、古いディスク内容で巻き戻さない。
        if (m_dirty) return;
        m_status = SaveStatus::IoError;
        try
        {
            if (m_path.empty()) throw std::runtime_error("LocalAppData unavailable");
            if (!std::filesystem::exists(m_path))
            {
                m_status = SaveStatus::Missing;
                return;
            }
            // MyLib.JsonIOはpath.string()へ変換するため、ここではUnicode対応のstreamを使う。
            std::ifstream input(m_path, std::ios::binary);
            if (!input) throw std::runtime_error("Cannot open save");
            m_status = SaveStatus::Corrupt;
            const auto data = nlohmann::json::parse(input);
            if (!file::jsonValue::Member(data, "version").is_number_integer()) return;
            if (file::jsonValue::Member(data, "version") != CampaignProgress::Version)
            {
                m_status = SaveStatus::Unsupported;
                return;
            }
            auto progress = CampaignProgress::FromJson(data);
            for (const auto& id : progress.cleared)
                if (std::find(knownIds.begin(), knownIds.end(), id) == knownIds.end())
                    throw std::runtime_error("Unknown cleared quest");
            m_progress = std::move(progress);
            m_status = SaveStatus::Valid;
            m_writeFailed = false;
        }
        catch (...)
        {
            // パース開始前の失敗は入出力異常、開始後は不正内容として区別する。
            if (m_status != SaveStatus::Corrupt) m_status = SaveStatus::IoError;
        }
    }

    bool CampaignSession::Write(const CampaignProgress& progress)
    {
        // MyLib.JsonIO::Writeは書込/close失敗の確認と置換を持たないため、保存境界だけ補う。
        // 本体を先に消さず、一時ファイルが完成した場合だけ同一ボリューム上で置き換える。
        HANDLE output = INVALID_HANDLE_VALUE;
        std::filesystem::path temporary;
        try
        {
            if (m_path.empty()) throw std::runtime_error("LocalAppData unavailable");
            std::filesystem::create_directories(m_path.parent_path());
            temporary = m_path;
            temporary += L".tmp";
            const auto bytes = progress.ToJson().dump(2);
            output = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL, nullptr);
            if (output == INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot create temporary save");
            DWORD written = 0;
            if (!WriteFile(output, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) ||
                written != bytes.size() || !FlushFileBuffers(output))
                throw std::runtime_error("Cannot flush save");
            const bool closed = CloseHandle(output) != 0;
            output = INVALID_HANDLE_VALUE;
            if (!closed || !MoveFileExW(temporary.c_str(), m_path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                throw std::runtime_error("Cannot replace save");
            m_writeFailed = false;
            return true;
        }
        catch (...)
        {
            if (output != INVALID_HANDLE_VALUE) CloseHandle(output);
            // 失敗した一時ファイルだけを除去し、既存本体には触れない。
            if (!temporary.empty()) DeleteFileW(temporary.c_str());
            m_writeFailed = true;
            return false;
        }
    }

    bool CampaignSession::StartNew()
    {
        const CampaignProgress initial;
        if (!Write(initial)) return false;
        m_progress = initial;
        m_status = SaveStatus::Valid;
        m_dirty = false;
        return true;
    }

    void CampaignSession::RecordClear(const std::string& id)
    {
        if (!Ready()) throw std::logic_error("Campaign must be initialized before departure");
        if (m_progress.cleared.insert(id).second) m_dirty = true;
        Save();
    }

    bool CampaignSession::Save()
    {
        if (!m_dirty) return true;
        if (!Write(m_progress)) return false;
        m_dirty = false;
        return true;
    }

    std::string CampaignSession::Message() const
    {
        if (m_writeFailed) return "保存できません。書込権限・空き容量を確認し、再試行してください。";
        switch (m_status)
        {
        case SaveStatus::Missing: return "攻略記録がありません。「最初から」で開始してください。";
        case SaveStatus::Corrupt: return "攻略記録が破損・不正です。ファイルは変更していません。";
        case SaveStatus::Unsupported: return "非対応のセーブ形式です。対応するゲーム版で開いてください。";
        case SaveStatus::IoError: return "攻略記録を読み込めません。保存先の権限を確認してください。";
        case SaveStatus::Valid: return "攻略記録を使用できます（遠征途中の再開ではありません）。";
        default: return "攻略記録を読み込みます。";
        }
    }
}
