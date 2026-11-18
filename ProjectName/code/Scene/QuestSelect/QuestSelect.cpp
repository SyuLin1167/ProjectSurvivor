module;
#include <DxLib.h>
module Scene.QuestSelect;
import Scene.Title;
import Scene.Play;
import Quest.QuestText;
import UI.Button;
import MyLib.Shape.Rect;
import <exception>;

namespace scene
{
    QuestSelect::QuestSelect()
    {
        // 全必須データを先に読み、途中失敗した一覧からの出発を防ぐ。
        try
        {
            for (const auto& entry : quest::AvailableQuests)
                m_quests.push_back(quest::FindQuest(entry.id));
            float y = 400;
            for (size_t i = 0; i < m_quests.size(); ++i)
            {
                ui::ButtonDef choice(quest::DisplayText(m_quests[i].name),
                    shape::Rect{{640, y}, {640, 52}}, GetColor(160, 178, 190));
                choice.onReleased = [this, i] { m_selected = i; };
                m_ui.Create<ui::Button>(&choice);
                y += 64;
            }
            ui::ButtonDef depart("Start", shape::Rect{{760, 780}, {160, 50}}, GetColor(160, 178, 190));
            depart.onReleased = [this] { m_depart = true; };
            m_ui.Create<ui::Button>(&depart);
            ui::ButtonDef back("Back", shape::Rect{{1000, 780}, {160, 50}}, GetColor(160, 178, 190));
            back.onReleased = [this] { m_back = true; };
            m_ui.Create<ui::Button>(&back);
        }
        catch (const std::exception& error)
        {
            ErrorLogFmtAdd("QuestSelect: %s", error.what());
            m_failed = true;
        }
    }

    SceneCmd QuestSelect::Update()
    {
        if (m_failed) return CmdQuit{};
        m_ui.Update();
        if (m_back) return CmdReplace{ [] { return std::make_shared<Title>(); } };
        if (m_depart) return Play::Start(shared_from_this(), m_quests[m_selected].id);
        return std::monostate{};
    }

    void QuestSelect::Draw()
    {
        if (m_failed) return;
        const auto& selected = m_quests[m_selected];
        DrawBox(600, 320, 1320, 850, GetColor(28, 35, 45), TRUE);
        DrawString(640, 350, "護送依頼を選択 / 目的地：街道", GetColor(255, 255, 255));
        DrawString(640, 570, quest::DisplayText(selected.name).c_str(), GetColor(255, 238, 188));
        DrawString(640, 610, quest::DisplayText(selected.description).c_str(), GetColor(255, 255, 255));
        DrawFormatString(640, 650, GetColor(255, 255, 255), "初期HP %.0f / 最大HP %.0f",
            selected.initialHealth, selected.maxHealth);
        if (selected.spawnInterval)
            DrawFormatString(640, 690, GetColor(255, 255, 255), "敵生成間隔：%.1f秒", *selected.spawnInterval);
        else DrawString(640, 690, "敵生成条件：基本設定を継承", GetColor(255, 255, 255));
        m_ui.Draw();
    }
}
