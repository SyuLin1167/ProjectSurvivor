# Issue #28: 終了診断と回帰確認

## 修正と根拠

`Execution::Run`がシーン・管理クラス・DxLibの生存中にCRT検査を行い、その結果1を終了コードへ返していた。Title ExitはCmdQuit、ウィンドウ終了はProcessMessageから同じRun終了経路へ合流する。

破棄順はシーン/オブジェクト → AppContext → Library/DxLib → 静的オブジェクト。Execution破棄直後もWindow等は生存するため、DebugのWinMainで`_CRTDBG_LEAK_CHECK_DF`を既存フラグへ追加し、CRT標準の終了時検査に任せる。Runは初期化失敗-1、ループ終了0を返す。**終了コードとCRT出力は別々に確認する。0だけでリークなしとは判定しない。** 破棄順・ゲーム仕様は変更しない。

## 検証記録（2026-10-21 JST）

対象: main `35fcc2b` → 本番修正 `2a12b5d`。Windows / VS 2026 Community 18.10.3 / v143 14.44.35207 / SDK 10.0.26100.0 / x64 Debug (/MTd)、DxLib 3.24f。

| 再現条件（実基盤をリンクした一時調査プログラム） | 修正前→後 | 静的破棄後のCRT報告 |
|---|---|---|
| Execution生成後に同じスレッドへPostQuitMessageを予約してRun | 終了1→0 | 両方なし |
| 上記＋Play(true)生成・Init・破棄を10回 | 終了1→0 | 両方なし |
| 対照として37 bytesを意図的に未解放 | 終了1→0 | 両方で37 bytesを検出 |

通常ブロックは開始前34→生存中117→Execution破棄後35件。静的破棄後に報告は消えた。これは今回の再現範囲の結果であり、全経路のリーク不在を保証しない。テスト側で終了時検査を有効にしたため、本番WinMainの実行確認はE1〜E4に残る。

ゲーム本体・調査プログラムのビルド成功。既存警告C4018/C4244/C4267/C4819/C5260あり。今回の整理では本番2ファイルを変更していないため、この実測を引き継ぐ。

詳細証拠・一時調査ソースは[整理前の固定コミット](https://github.com/SyuLin1167/ProjectSurvivor/tree/5e6dc7b45e17a54b8e133e7f37e1216592e3ab92)に保存。必要時だけ復元して再調査する。専用プロジェクト・スクリプト・詳細ログを恒久保守しない。

## 維持する最小回帰テスト: E1〜E4（手動）

修正済みコミットを**x64 Debug**でビルドし、Visual Studioのデバッガーで起動。「出力 → デバッグ」を表示する。各ケースは別プロセスで実行し、停止ボタンで強制終了しない。

| ID | 操作 | 確認 |
|---|---|---|
| E1 | Title表示後にExitをクリック | 終了コード・CRT出力・例外/停止の有無 |
| E2 | Title表示後にウィンドウ× | 同上。E1との差 |
| E3 | Start→試作を操作→RでTitle→Exit | 同上。シーン遷移後の解放 |
| E4 | 試作画面でウィンドウ× | 同上。Player/Base/Collider保持中からの解放 |

期待: 通常終了0、終了時CRTリーク報告なし、例外/停止なし。**0でもCRT報告があれば要調査。出力未取得は成功扱いにしない。** 日時・対象SHA・構成・環境と、E1〜E4それぞれの終了コード/CRT出力/例外を[Issue #28](https://github.com/SyuLin1167/ProjectSurvivor/issues/28)へ記録し、異常時は出力全文を添付する。終了・所有構造を変更した際の回帰確認にも使用する。

2026-10-21追記: ユーザーがE1〜E4すべて終了コード0、`Alloc memory dump Total size:0(0.000kb) Alloc num:0`を確認。PR #30承認・マージ済み。開発AIによるネイティブ操作は未実施。初期化失敗注入、ロード途中終了、長時間反復、CRT外リソースは未実施。Release未検証・x86保留。

## 整理と次工程

本番main.cpp/Execution.cpp、上記回帰手順・結果要約を維持。一時調査3ファイル（ExitDiagnostics.cpp、RunExitDiagnostics.ps1、ExitDiagnostics.vcxproj）とIssue28詳細ログ3ファイルは削除。専用基盤を追加せず、本番経路の手動確認と既存テストを使う。

既存のMovementTests/BaseMotionTests/RunMovementTests.cmdは移動・JSON・時間計算の回帰、BaseColliderTests.cpp/.vcxprojは実Colliderとの統合回帰に再利用でき、確認範囲も異なるため維持する。過去Issueの検証記録は対応構成・実測の履歴として維持し、同種の詳細ログの追加は原則行わない。ゲーム実装に今回の整理のための抽象化や変更は加えない。

#28はユーザー検証とPR #30マージにより完了。#6へ進行。E1〜E4は今後の終了経路変更時の回帰手順として維持する。
