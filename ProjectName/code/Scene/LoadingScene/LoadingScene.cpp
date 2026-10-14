module;
#include <DxLib.h>

module Scene.LoadingScene;
import GameSystem.FrameRate;
import MyLib.Loading;
import MyLib.Loading.LoadingContext;
import Scene.Title;

namespace scene
{
    LoadingScene::LoadingScene(int8_t policy, std::function<std::shared_ptr<SceneBase>()> nextScene, std::shared_ptr<SceneBase> holdScene)
        : m_policy(policy)
        , m_loadScene(nextScene)
        , m_holdScene(std::move(holdScene))
        , m_coroutine()
    {
        m_objManager.reset();

        // ローディング開始
        SetUseASyncLoadFlag(true);
        task::LoadingContext::Set(std::make_unique<task::Loading>().release());
        task::LoadingContext::Get()->StartLoading();

        // ポリシー別処理
        if (m_policy & (LoadPolicy::COROUTINE | LoadPolicy::PROGRESS))
        {
            m_coroutine = std::move(LoadCoroutine());
        }
        else if (m_policy & LoadPolicy::DONE)
        {
            m_nextScene = m_loadScene();
            // タスク登録完了後に実行開始
            task::LoadingContext::Get()->Run();
        }
    }

    LoadingScene::~LoadingScene()
    {
        SetUseASyncLoadFlag(false);
        task::LoadingContext::Release();
    }

    Coroutine LoadingScene::LoadCoroutine()
    {
        // 次シーンのローディング処理
        m_nextScene = m_loadScene();

        // タスク登録完了後に実行開始
        task::LoadingContext::Get()->Run();

        // 読み込み完了まで待機
        co_yield WaitUntil{ [this] { return !task::LoadingContext::Get()->IsLoading(); } };

        // 失敗時は100%表示待ちを省略して終了
        if (task::LoadingContext::Get()->HasFailed())
        {
            m_failed = true;
            co_return;
        }

        // 100％表示用の待機時間
        co_yield WaitForSeconds{ 0.5f };

        // 読み込み完了
        co_return;
    }

    SceneCmd LoadingScene::OnLoadFailed()
    {
        // 失敗理由をログ出力
        ErrorLogFmtAdd("Loading failed: %s", task::LoadingContext::Get()->FailureReason().c_str());

        // タイトル自体の読み込み失敗(戻り先なし)は安全に終了
        if (!m_holdScene)
        {
            return CmdQuit{};
        }

        // それ以外はタイトルへ戻る
        return CmdReplace{ []() { return std::make_shared<Title>(); } };
    }

    SceneCmd LoadingScene::Update()
    {
        // ワーカーが予約した共有コンテナーの更新をメインスレッドで反映
        if (auto* loading = task::LoadingContext::Get())
        {
            loading->PumpMain();
        }
        // ポリシーに基づいた行進処理
        if (m_policy & (LoadPolicy::COROUTINE | LoadPolicy::PROGRESS))
        {
            // コルーチン実行
            if (m_coroutine.handle)
            {
                // 待機処理
                auto yieldType = m_coroutine.get_yield_type();
                if (std::holds_alternative<WaitForSeconds>(yieldType))
                {
                    // 指定時間待機
                    if (auto waitPtr = std::get_if<WaitForSeconds>(&yieldType))
                    {
                        static float elapsed = 0.0f;
                        elapsed += gameSystem::FrameRate::GetDeltaTime();
                        if (elapsed < waitPtr->seconds)
                        {
                            return std::monostate{};
                        }
                    }
                }
                else if (std::holds_alternative<WaitUntil>(yieldType))
                {
                    // 指定条件完了確認
                    if (auto waitPtr = std::get_if<WaitUntil>(&yieldType))
                    {
                        if (!waitPtr->pred())
                        {
                            return std::monostate{};
                        }
                    }
                }

                // コルーチン再開
                m_coroutine.resume();

                // コルーチン完了確認
                if (m_coroutine.handle.done())
                {
                    task::LoadingContext::Get()->PumpMain();
                    if (m_failed)
                    {
                        return OnLoadFailed();
                    }
                    auto next = m_nextScene;
                    return CmdReplace{ [next]() { return next; } };
                }
            }
        }
        else if (m_policy & LoadPolicy::DONE)
        {
            // 読み込み完了確認
            if (!task::LoadingContext::Get()->IsLoading())
            {
                task::LoadingContext::Get()->PumpMain();
                if (task::LoadingContext::Get()->HasFailed())
                {
                    m_failed = true;
                    return OnLoadFailed();
                }
                auto next = m_nextScene;
                return CmdReplace{ [next]() { return next; } };
            }
        }

        // 継続（何もしない）
        return std::monostate{};
    }

    void LoadingScene::Draw()
    {
        if (m_holdScene)
        {
            if (m_policy & LoadPolicy::PROGRESS)
            {
                m_holdScene->Draw();
            }

            m_holdScene->DrawLoading();
        }
    }
}
