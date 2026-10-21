#include<Windows.h>
#include <crtdbg.h>
#include <memory>
import GameSystem.Execution;

/// <summary>Debugの終了時メモリー診断を設定し、ゲームの実行結果を返す。</summary>
/// <param name="hInst">現在のアプリケーションインスタンス（本処理では未使用）。</param>
/// <param name="hInstPrev">以前のインスタンス。Windowsでは常にNULL（未使用）。</param>
/// <param name="cmdline">実行ファイル名を除いたコマンドライン（未使用）。</param>
/// <param name="cmdshow">初期表示状態の指定（未使用）。</param>
/// <returns>Execution::Runの終了コード。通常終了は0。</returns>
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hInstPrev, _In_ PSTR cmdline, _In_ int cmdshow)
{
#ifdef _DEBUG
    // Run内やExecution破棄直後では、Windowやタグ表などがまだ生存している。
    // CRT標準の終了時検査で静的オブジェクト破棄後に診断し、既存の検査フラグも保持する。
    _CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
    // 戻り値はゲーム実行結果。unique_ptrはこの式の評価後に破棄され、
    // 上の終了時検査より先にシーン、管理クラス、DxLibが解放される。
    return std::make_unique<gameSystem::Execution>()->Run();
}
