module;
#include <DxLib.h>
module Scene.QuestSelect;
import Scene.Title;
import Scene.Play;
import Quest.QuestText;
import UI.Button;
import MyLib.Shape.Rect;
import <exception>;
import <algorithm>;

namespace scene
{
    QuestSelect::QuestSelect()
    {
        // 全必須データを先に読み、途中失敗した一覧からの出発を防ぐ。
        try
        {
            for (const auto& [id, entry] : quest::AvailableQuests)
                m_quests.push_back(quest::FindQuest(id));
            // ハッシュ表の走査順に依存せず、表示順と先頭の初期選択を毎回そろえる。
            std::sort(m_quests.begin(), m_quests.end(), [](const auto& left, const auto& right) {
                const int leftOrder = quest::AvailableQuests.at(left.id).displayOrder;
                const int rightOrder = quest::AvailableQuests.at(right.id).displayOrder;
                return leftOrder != rightOrder ? leftOrder < rightOrder : left.id < right.id;
            });
            // 表示順に依頼ボタンを配置する。配列は構築後に変えず、選択位置を添字で保持する。
            float y = 400;
            for (size_t i = 0; i < m_quests.size(); ++i)
            {
                ui::ButtonDef choice(quest::DisplayText(m_quests[i].name),
                    shape::Rect{{640, y}, {640, 52}}, GetColor(160, 178, 190));
                choice.onReleased = [this, i] { m_selected = i; };
                m_ui.Create<ui::Button>(&choice);
                y += 64;
            }
            // コールバックは要求だけを記録し、UI走査終了後のUpdateでシーンを切り替える。
            ui::ButtonDef depart("Start", shape::Rect{{760, 780}, {160, 50}}, GetColor(160, 178, 190));
            depart.onReleased = [this] { m_depart = true; };
            m_ui.Create<ui::Button>(&depart);
            // 戻る操作は新しいTitleを作り、今回の選択を次回へ持ち越さない。
            ui::ButtonDef back("Back", shape::Rect{{1000, 780}, {160, 50}}, GetColor(160, 178, 190));
            back.onReleased = [this] { m_back = true; };
            m_ui.Create<ui::Button>(&back);
        }
        catch (const std::exception& error)
        {
            // 読込途中のエラーをログへ残し、未完成の一覧を描画・操作せず終了する。
            ErrorLogFmtAdd("QuestSelect: %s", error.what());
            m_failed = true;
        }
    }

    SceneCmd QuestSelect::Update()
    {
        // 必須データが揃わなかった場合は、UI更新に入る前に終了する。
        if (m_failed) return CmdQuit{};
        m_ui.Update();
        // 同じ更新で両要求を受けた場合は戻る操作を優先する。
        if (m_back) return CmdReplace{ [] { return std::make_shared<Title>(); } };
        // 選択IDを値で渡し、新規Play側で依頼と基本設定を再読込する。
        if (m_depart) return Play::Start(shared_from_this(), m_quests[m_selected].id);
        return std::monostate{};
    }

    void QuestSelect::Draw()
    {
        // 読込失敗時は未完成の配列を参照しない。正常時の選択添字はボタン登録時に保証する。
        if (m_failed) return;
        const auto& selected = m_quests[m_selected];
        // 背景板と選択中の詳細を描き、出発前に依頼内容とHPの差を確認できるようにする。
        DrawBox(600, 320, 1320, 850, GetColor(28, 35, 45), TRUE);
        DrawString(640, 350, "護送依頼を選択 / 目的地：街道", GetColor(255, 255, 255));
        DrawString(640, 570, quest::DisplayText(selected.name).c_str(), GetColor(255, 238, 188));
        DrawString(640, 610, quest::DisplayText(selected.description).c_str(), GetColor(255, 255, 255));
        DrawFormatString(640, 650, GetColor(255, 255, 255), "初期HP %.0f / 最大HP %.0f",
            selected.initialHealth, selected.maxHealth);
        // 間隔の指定がある場合は秒数を示し、省略時は基本設定を継承することを表示する。
        if (selected.spawnMode == "offscreen")
            DrawFormatString(640, 690, GetColor(255, 255, 255), "画面外から段階生成 / 同時上限 %d体", selected.spawnLimit.value_or(20));
        else if (selected.spawnInterval)
            DrawFormatString(640, 690, GetColor(255, 255, 255), "敵生成間隔：%.1f秒", *selected.spawnInterval);
        else DrawString(640, 690, "敵生成条件：基本設定を継承", GetColor(255, 255, 255));
        // このシーンが所有するボタンを最後に描画し、詳細表示の上から操作できるようにする。
        m_ui.Draw();
    }
}
