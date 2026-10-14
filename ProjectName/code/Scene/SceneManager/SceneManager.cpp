module;
#include <DxLib.h>

module Scene.SceneManager;

import MyLib.KeyStatus;
import MyLib.MouseStatus;
import Scene.Title;
import GameSystem.FrameRate;
import <memory>;
import <variant>;

using namespace gameSystem;
using namespace input;

namespace scene
{
    SceneManager::SceneManager()
        : m_pendingCmd()
        , m_isRunning(true)
    {
        //最初のシーンをタイトルに設定
        m_nowScene.emplace(std::make_shared<Title>());
    }

    void SceneManager::GameLoop()
    {
        //ゲームループ
        while (!ProcessMessage() && m_isRunning)
        {
            Update();
            Draw();
            ChangeScene();
        }
    }

    void SceneManager::Update()
    {
        //更新処理
        FrameRate::CalcFrameRate();
        KeyStatus::UpdateKeyState();
        MouseStatus::UpdateMouseState();
        m_pendingCmd = m_nowScene.top()->Update();
    }

    void SceneManager::Draw()
    {
        //描画
        ClearDrawScreen();
        clsDx();
        m_nowScene.top()->Draw();

#ifdef _DEBUG
        FrameRate::DrawFrameRate();
#endif // _DEBUG
        ScreenFlip();
    }

    void SceneManager::ChangeScene()
    {
        // フレーム終端でコマンド適用
        std::visit(
            [this](auto&& cmd)
            {
                using T = std::decay_t<decltype(cmd)>;
                if constexpr (std::is_same_v<T, std::monostate>)
                {
                    // 何もしない
                }
                else if constexpr (std::is_same_v<T, CmdPush>)
                {
                    // 新しいシーンを積む
                    auto next = cmd.build ? cmd.build() : nullptr;
                    if (next)
                    {
                        m_nowScene.push(next);
                        m_nowScene.top()->Init();
                    }
                }
                else if constexpr (std::is_same_v<T, CmdReplace>)
                {
                    // 現在のシーンを置き換える
                    auto next = cmd.build ? cmd.build() : nullptr;
                    if (next)
                    {
                        if (!m_nowScene.empty())
                        {
                            m_nowScene.pop();
                        }
                        m_nowScene.push(next);
                        m_nowScene.top()->Init();
                    }
                }
                else if constexpr (std::is_same_v<T, CmdPop>)
                {
                    // 現在のシーンを取り除く
                    if (!m_nowScene.empty())
                    {
                        m_nowScene.pop();
                    }
                }
                else if constexpr (std::is_same_v<T, CmdQuit>)
                {
                    // ゲーム終了
                    PostQuitMessage(0);
                    m_isRunning = false;
                }
            },
            m_pendingCmd);

        // コマンドをクリア
        m_pendingCmd = std::monostate{};
    }
}