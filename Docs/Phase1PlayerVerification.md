# Issue #4: 仮ステージ・独立プレイヤーの実装と確認

実装基準: main 689dfc6865d4837b413f1d374a18292e9cfd2721。ブランチ: issue/4-player-stage。
検証日: 2026-10-19 JST。対象の成果コミットはPRに記録する。
状態: **実装・ビルド・計算テスト済み、目視/操作結果待ち。Issueは未完了、PRはDraft。**

## 承認範囲と今回の変更

[DEC-01回答](https://github.com/SyuLin1167/ProjectSurvivor/issues/3#issuecomment-6251280953)は2026-10-18、プロジェクトオーナー（企画担当）によるPhase 1試作限定承認。
[仕様台帳](SpecificationDecisions.md)へD1～D6、仮値、調整委任と未確定事項を転記した。正式仕様へ昇格しない。

- TitleのStartから仮Playへ進む。既存Scene/Loading/ObjectManager/Player/Mapを使う。
- 青い32×32uのP。中心(512,2048)、240u/s。WASDと矢印は同じ入力として扱い、同方向重複は加速なし、逆方向は軸ごと相殺。斜め正規化、離すと停止。Jump/重力/音声/アニメは試作へ接続しない。
- 固定16384×4096uの空き地、128uグリッド・境界線。空き地にはタイルデータも画像も不要なため、Mapに試作描画経路を追加し、元のJSON/CSV/TileCollider経路は保持。無限/自動生成は行わない。
- RectColliderは左上座標なので、承認された中心から半サイズを引いて初期化。移動予定量を境界内へ制限し、既存ColliderManager::Stepで一度だけ座標更新。中心の可動範囲はX=16～16368、Y=16～4080。試作MapはTileColliderを生成せず、R6の二重移動経路を使用しない。基盤のタイル問題を修正済みとは扱わない。
- Playerの中心へ直接追従するCameraの小変更を含む。全マップで操作確認を可能にするための最小接続で、境界clamp/WorldToScreenは既存を再利用。旧1/3位置・補間はサンプル経路に保持。RでTitleへ戻り再開始できる。
- Play(false)、Player()/Map()は従来サンプル経路を保持。製品向けのモード切替UIは追加しない。旧経路はコンパイル済みだが今回の操作回帰は未実施。

今回の速度調整なし。拠点追加は#5、画面外拠点表示と統合カメラ検証は#6、再合流検証は#7。拠点がまだないのでX=15000uの試行終了も未実装で、今回のPの移動境界とは区別する。
敵・Goal・勝敗・HP・正式アセットの試作への追加は行っていない。

## 検証結果

環境は[BuildBaseline](BuildBaseline.md)のVS Community 2026 / v143 14.44.35207 / Windows SDK 10.0.26100.0、x64 Debug。
DxLib 3.24fと固定JSONを既存の検証用コピーから新しいworktreeへ複製。setup.batは再実行せず、ユーザーのDesktopコピーも変更していない。
json.hpp SHA-256: 16a9185f661eadb85cde13b5b995e9c54f92c71044a743dc6d0e1b4346666af8。

| 確認 | 結果と限界 |
|---|---|
| x64 Debugビルド | 成功、終了コード0。C4819/C4244/C4018等の警告あり。[ログ](Verification/Issue4/build-debug-x64.txt) |
| 制限内ビルド | C1902で失敗。同じ構成を制限外で再実行して成功。設定変更で回避していない |
| 移動計算テスト | 成功。16入力組合せ、30/60/120fps相当で2秒の縦横/斜め480u（誤差0.1u未満）、四辺の超過・保持・脱出、辺に沿う移動、停止、旧横入力を確認。[結果](Verification/Issue4/movement-tests.txt) |
| 起動観測 | 5秒後に2DGame、Responding=True。終了要求後にプロセス終了。ただしExitCode=1。[記録](Verification/Issue4/runtime.txt) |
| 実際の表示・キー操作・再開始 | **未実施**。本セッションのネイティブ画面操作は無効で、従来の分担に従いユーザーが下表を確認する。計算テストは入力取得/描画/シーン統合を保証しない |
| x86 / Release | 既存設定のまま保留 / 未検証 |

終了コード1を正常終了扱いにしない。既存Execution::RunがExecutionの破棄前に_CrtDumpMemoryLeaks()を返す点は確認したが、今回の戻り値との因果は未切分け。終了経路は変更していない。ユーザー確認でも再現する場合は独立した問題として切り分ける。

計算テストの再現（リポジトリルート、x64 Native Tools Command Prompt / v143）:

```bat
mkdir .verification
cl /nologo /EHsc /std:c++20 /utf-8 Tests\MovementTests.cpp /Fo.verification\MovementTests.obj /Fe.verification\MovementTests.exe
.verification\MovementTests.exe
```

[MovementTests.cpp](../Tests/MovementTests.cpp)はゲームが使用するMovementMath.hを直接テストする。OSのキー取得、Collider Stepそのもの、描画はこのテストの対象外。

## ユーザー確認手順

PRのブランチを新しい作業コピーで取得し、[BuildBaseline](BuildBaseline.md)の依存導入・ビルド手順でDebug/x64をビルドする。依存配置済みコピーではsetup.batを再実行しない。
本作業コピーのProjectName/x64/Debug/project.exeも同じ内容でビルド済み。exeを単体で移動せず、その場所から起動する。

| ID | 操作 | 期待する結果 |
|---|---|---|
| U1 | 起動→Start | 空き地グリッド・青いP・操作案内。中心表示(512,2048)、速度240。敵・Goal・拠点はまだ表示しない |
| U2 | WASD各方向、矢印各方向を押して離す | 該当方向へ移動、離すと座標停止。片方のキー配置しか動かない状態がない |
| U3 | W+D等の4斜め方向 | 縦横より速くならない。内部計算は正規化済み。方向と見た目の違和感を確認 |
| U4 | D+右、W+上、A+D、W+S、W+S+D | 重複は加速なし、反対方向は停止、最後の組合せは右のみ。すべて離すと停止 |
| U5 | Space、無入力で待つ | ジャンプや落下が起きない |
| U6 | 左/右/上/下の境界、4隅へ進む | 中心X16～16368、Y16～4080で停止。P全体が内側。辺で斜め入力すると辺に沿う。逆方向なら離れられる（右端までは初期位置から約66秒） |
| U7 | マップ中央へ移動→端へ移動 | 中央付近はP中心追従。端付近ではカメラが止まりPが画面内を移動。グリッド・Pがずれない |
| U8 | 移動後R→Title→Startを2回 | 初期中心(512,2048)とカメラへ戻る。古いPが残らない |
| U9 | R→Title→Exit | ウィンドウが閉じ、例外表示なし。取得可能なら終了コードも報告（AIの終了要求時は1、未解消） |

回答時は対象コミット、U1～U9の成功/失敗/未実施、失敗した操作と表示座標、画像、終了コードを記録してください。未実施を成功扱いにせず、結果反映後にIssue #4の完了条件を再判定する。
目視・操作結果待ちの間はIssue #4をBlocked、PRをDraftとし、#5以降は先行Issue完了待ちのBacklog。ユーザー承認なしにマージしない。
