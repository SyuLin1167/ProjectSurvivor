# Issue #28: 終了コードとメモリー診断の切り分け

調査日: 2026-10-21 JST / 開発担当AI。
対象main: `35fcc2bbccc2cb620bbd4512a65808907989bad0`（PR #29マージ後）。
修正・テストソース: `2a12b5d085bbf43ee930e47cac98b7b287bed534`。
[Issue #28](https://github.com/SyuLin1167/ProjectSurvivor/issues/28) /
[ユーザーの手動確認](https://github.com/SyuLin1167/ProjectSurvivor/issues/28#issuecomment-6293965588)。

## 結論と適用範囲

**確認できた不具合は、所有中のメモリーを検査して、その結果をゲームの終了コードとして返していたこと。**
終了コード1のみから実リークを断定できない。今回の実基盤による再現では、静的オブジェクト破棄後のCRT検査にリーク報告はなかった。これは全操作経路、全ライブラリ、全種類のリソースについてリーク不在を保証するものではない。

修正は終了時診断の時期と戻り値の責務に限定する。ゲームの移動・シーン遷移・破棄順序・既存の所有構造は変更しない。Issue #6の実装は含まない。

**未完了:** 修正済みゲーム本体でのTitle Exitクリックとウィンドウ×による終了コード・CRT出力の比較。ネイティブ画面操作APIが利用できないため未実施。既存U1〜U9/B1〜B7成功の報告は維持するが、この修正後の数値確認に代用しない。

## コード経路

| 経路 | ゲームループを抜けるまで | 修正前の最終結果 |
|---|---|---|
| Title Exit | ButtonのonReleased → `m_toExit` → `CmdQuit` → `SceneManager::ChangeScene`で`PostQuitMessage(0)`と`m_isRunning=false` | `GameLoop`の後にCRT検査結果を返す |
| ウィンドウ終了要求 | `GameLoop`の`ProcessMessage()`が非0となりループを抜ける | 同じCRT検査結果を返す |
| Library初期化失敗 | `InitSuccess()==false` | `Run`は-1。今回維持、失敗注入は未実施 |

`GameLoop`はvoidで、`PostQuitMessage(0)`の0を`WinMain`へ返しているわけではない。両終了経路の後に`Execution::Run`が`_CrtDumpMemoryLeaks()`を呼び、その戻り値が`WinMain`へ伝播していた。

`WinMain`の`return std::make_unique<Execution>()->Run();`では、Runの戻り値を評価した**後**に一時unique_ptrが破棄される。したがって、検査時はExecution自身、SceneManager、AppContext、DxLibが生存している。

根拠:
[main.cpp](../ProjectName/code/main.cpp)、
[Execution.cpp](../ProjectName/code/GameSystem/Execution/Execution.cpp)、
[Title.cpp](../ProjectName/code/Scene/Title/Title.cpp)、
[SceneManager.cpp](../ProjectName/code/Scene/SceneManager/SceneManager.cpp)。

## 破棄順序と残存割り当て

1. `Execution::~Execution`で`m_scene.reset()`。SceneManagerの保有シーンを解放。Titleは画像ハンドルとUIボタンを解放し、SceneBaseのObjectManagerも破棄される。
2. ObjectManager内ではオブジェクトがColliderManagerより先に破棄される。LoadingSceneを保有している場合はLoadingContextをReleaseし、Loadingのデストラクターがワーカー終了を待つ。今回の再現は非同期ロード中の終了試験ではない。
3. `AppCtx::Deactivate()`でFileSystem、AssetManager、UIManagerの順にreset。シーンのデストラクターが使用する管理クラスはこの時点まで生存する。
4. Executionのメンバー破棄でLibraryを解放し、`DxLib_End()`を呼ぶ。
5. さらに後の静的終了処理でWindowのsingleton、キー配列、ObjectTagの変換表などを破棄する。

Execution破棄直後にもCRT検査は1を返した。開始前の通常ブロック34件/1472 bytesに対し、破棄直後は35件/1484 bytes。追加12 bytesのダンプ内容は1920、1080、32で、Window::Infoの設定と一致し、コードでもstatic Windowがunique_ptrで保持している。Windowの寿命終了前なので未解放は予定どおりである。

開始前からの34件について各割り当てのスタック追跡は行っていない。静的コンテナーが存在することと、**静的破棄後にはこれらの報告が消えることを実測**した。割り当て番号やアドレスは実行ごとに変わるため、識別子固定のテストはしない。

## 最小修正

- Debugの`WinMain`で現在のCRTフラグに`_CRTDBG_LEAK_CHECK_DF`を追加。標準のプロセス終了時検査を有効化し、静的オブジェクトの破棄も待つ。既存フラグは上書きしない。
- `Run`では初期化失敗-1を維持し、ゲームループ終了は既存の宣言どおり0を返す。
- 診断はVisual Studioの「出力 → デバッグ」へ残す。検査そのものの削除、割り当ての除外、破棄順の変更は行わない。

**終了コードはゲーム実行結果、CRT出力はメモリー診断として別々に確認する。** 終了時診断の戻り値はプロセスの終了コードへ変換しない。このため意図的リークの対照試験でもゲーム結果は0だが、37 bytesのリーク報告は残り、テストはその報告を検証する。「0ならリークなし」という運用にしない。

新しいヘッダーや抽象化は追加していない。変更先は既存の実装.cppとエントリーポイント、テストの.cpp/.ps1/.vcxproj。C++モジュール優先方針を変更しない。編集した日本語コメント付きC++はUTF-8 BOMで保存した。

CRTの仕様・検査時期の根拠:
[_CrtDumpMemoryLeaks](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/crtdumpmemoryleaks)、
[CRTによるリーク検出・False positives](https://learn.microsoft.com/en-us/cpp/c-runtime-library/find-memory-leaks-using-the-crt-library)。
CRTはその時点の未解放ブロックを報告するため、生存中の静的オブジェクトを含む早期検査は誤認につながる。

## 実測

Windows / Visual Studio 2026 Community 18.10.3 / MSVC v143 14.44.35207 / Windows SDK 10.0.26100.0 / **x64 Debug、/MTd**。
DxLib 3.24fと既存nlohmann/jsonを検証済みコピーから再利用。setup.batは再実行していない。制限環境でビルドが進まず中断し、許可された通常のVisual Studio環境でビルドした。

| 再現条件 | 修正前Run/終了コード | 修正後Run/終了コード | 静的破棄後のCRT出力 |
|---|---|---|---|
| 実Execution生成後、同じスレッドへ終了メッセージを予約してRun | 1 / 1 | 0 / 0 | 前後とも報告なし |
| 上記に加え実Phase 1 Play(true)を10回生成・Init・破棄 | 1 / 1 | 0 / 0 | 前後とも報告なし |
| 対照: 終了直前にテスト専用37 bytesを未解放で残す | 1 / 1 | 0 / 0 | 前後とも37 bytesを検出 |

各ケースの通常ブロック数は開始前34 → Execution生存中117 → 破棄後35、サイズは1472 → 9117 → 1484 bytes。Execution破棄直後の手動CRT検査は前後とも1。ゲームコードを修正する前にもテスト側で終了時検査を有効化して比較した。

テストはゲームと同じ全ソース・依存をリンクし、エントリーだけ差し替える。**Titleのクリック・実ウィンドウ×・Playの実操作/描画・LoadingScene経由の遷移は試験していない。** `PostQuitMessage`によるProcessMessage経路と、実クラスの所有物解放を調べる再現である。テスト側でも終了時検査フラグを設定するため、ゲーム本体のWinMain設定の実行確認は下記手動確認に残る。

ゲーム本体・診断テストともビルド成功。既存コード由来のC4018/C4244/C4267/C4819/C5260警告は残り、今回の範囲では変更しない。診断ランナーは3条件すべてPASS、終了コード0。

ログ: [修正前後の実測](Verification/Issue28/results.txt)、[ゲーム本体ビルド](Verification/Issue28/game-build.txt)、[診断テストビルド](Verification/Issue28/test-build.txt)。

### 再現コマンド

リポジトリルート、Visual Studioのx64 Native Tools環境で:

```powershell
msbuild ProjectName/2DGameSample.sln /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=v143
msbuild ProjectName/project/ExitDiagnostics.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=v143
powershell -NoProfile -ExecutionPolicy Bypass -File Tests/RunExitDiagnostics.ps1
```

診断exeは`ProjectName/x64/Debug/ExitDiagnostics.exe`、記録先は`.verification/exit-results/`。
テスト用37 bytesの未解放は`--leak`の場合だけで、ゲームexeには含まれない。テストは終了コードに加え、終了時CRT報告の有無と37 bytesの対照検出を照合する。修正前ソースにテストだけを適用して比較する場合は`-ExpectedRunResult 1`。

## 残る手動確認

同じ修正済みコミットをx64 Debugでビルドし、Visual Studioのデバッガーで起動する。各ケースは別プロセスで実行し、停止ボタンによる強制終了は使わない。

| ID | 操作 | 記録する内容 |
|---|---|---|
| E1 | Title表示後にExitをクリック | 終了コード、終了時CRT出力全文、例外/停止の有無 |
| E2 | Title表示後にウィンドウ× | 同上。E1との差を比較 |
| E3 | Start→試作を操作→RでTitle→Exit | 同上。シーン遷移後も確認 |
| E4 | 試作画面でウィンドウ× | 同上。Player/Base/Colliderを保持した状態からの終了 |

期待はゲームループが通常終了してコード0、終了時リーク報告なし。**コード0でもCRT出力に報告があれば未解決**。出力が得られなければ未取得と記録し、成功扱いにしない。発生時は出力の割り当て番号を`_CrtSetBreakAlloc`等で追跡し、所有者と解放経路を調べる（番号は同じ条件でも変わり得る）。

記録様式:

```text
日時/JST:
対象コミット / x64 Debug / VS・SDK:
E1 終了コード / CRT出力 / 例外:
E2 終了コード / CRT出力 / 例外:
E3 終了コード / CRT出力 / 例外:
E4 終了コード / CRT出力 / 例外:
出力ログ添付:
未実施理由:
```

Release未検証、x86保留。初期化失敗注入、ロード途中終了、長時間反復、GPU等CRT外のリソース調査は未実施。既存基盤の正式な対応構成は変更しない。

## 次工程の提案

今回の不具合は終了時の診断と戻り値の扱いであり、移動・カメラ・追跡に重大な影響がある証拠は得られていない。既存のU1〜U9/B1〜B7成功も踏まえ、修正PRをレビューし、残る終了操作の結果を#28で管理したうえで、#6を次の開発候補とする。#28を完了扱いにせず、#7の統合報告でも残項目を再確認する。PRは自動マージせず、ユーザー判断を待つ。
