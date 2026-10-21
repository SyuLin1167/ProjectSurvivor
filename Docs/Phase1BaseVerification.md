# Issue #5: プレイヤーを待たない移動拠点

基準main: `36867b3245abbe760036a6cc5ed5e8fed05b904e`（PR #27マージ後）。
ブランチ: `issue/5-moving-base`。作業・検証日: 2026-10-20 JST。対象成果コミットはPRに記録。

## 承認範囲・実装

[DEC-01](SpecificationDecisions.md)のPhase 1試作限定承認に従う。配布JSONの承認初期値は変更していない。

- `MovingBase`は既存GameObject派生。ObjectManagerのタグ一覧にBASEを追加し、画面内外を問わず更新する。
- 橙色96×64u、文字`BASE ->`、中心(768,2048)、+Xへ80u/s。Pは240u/s、速度比3倍、相対速度160u/s。
- `BaseMotion.ixx`で方向決定と1フレーム変位計算を分離。プレイヤー入力・位置・カメラへの依存はない。経路管理システムは追加しない。
- 既存RectColliderに変位を渡し、ColliderManager::Stepで一度だけ座標更新、LateUpdateで位置を読む。双方の衝突マスクを登録せず、Pと拠点は相互通過する。TileCollider/triggerの既知リスク経路へ接続しない。
- 試行終端は中心X15000u。最終フレームの変位を残距離へ制限し、LateUpdate後に既存SceneCmdでTitleへ戻る。正式な勝敗やResultではない。標準値で約177.9秒。
- 画面外でも確認できる拠点中心座標・速度・終端Xを仮表示。方向/距離インジケータは#6へ残す。戦闘・HP・正式アセットは追加しない。

## JSON設定

正本: `ProjectName/resources/data/Phase1Config.json`。既存FileSystem/JsonIOを再利用し、Player/Map/MovingBase生成時に同じファイルを読む。

| キー | 初期値 | 単位・意味 |
|---|---:|---|
| baseWidth / baseHeight | 96 / 64 | u、仮拠点の矩形サイズ |
| baseSpeed | 80 | u/s、独立前進速度 |
| baseCenterX / baseCenterY | 768 / 2048 | u、初期中心 |
| trialEndX | 15000 | u、試行終了位置 |

既存のP・マップ・グリッド設定も維持。必須キーの欠損・型不正・非有限値・不正な矩形/終端を拒否する。
P180〜300、拠点60〜100、速度比2〜3を同時に確認し、委任外の速度設定も例外で拒否する。範囲外の仕様変更は事前判断が必要。
他の寸法・座標をJSONで編集できることは変更権限の拡大ではない。

設定変更時は元のJSONを編集してアプリを再起動する。既存基盤が起動時にexe横へコピーするため、exe横だけの編集は上書きされる。ホットリロードは行わない。
テスト内の200/100u/sは入力反映の検査用であり、配布設定の調整や試遊採用ではない。

## 検証方法と結果

Windows / VS2026 Community / v143 14.44 / SDK10.0.26100.0、x64 Debug。
依存は既存検証コピーのDxLib/jsonを複製。setup.bat、ユーザーDesktopコピー、x86/Release設定は変更していない。

1. 本体ビルド: `MSBuild.exe ProjectName/2DGameSample.sln /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=v143`
2. 移動/JSON計算テスト: リポジトリルートのx64 Native Tools Command Prompt（v143）で `Tests\RunMovementTests.cmd`
3. 実Colliderテスト: `MSBuild.exe ProjectName/project/BaseColliderTests.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=v143`、その後ルートから `.verification\BaseColliderTests.exe`

テスト用プロジェクトは本体プロジェクトのソース・依存・設定を取り込み、mainだけをテストに差し替える。中間・出力先は`.verification`へ分離し、ゲーム本体の成果物を上書きしない。

| 検証 | 実測・限界 |
|---|---|
| x64 Debug本体ビルド | 成功、終了0。既存C4819/C4244/C4018等の警告あり |
| 移動・設定テスト | 成功。既存Pの入力16組合せ/境界テストを含む |
| 拠点60秒、30fps相当 | 4799.96u、理論4800uとの差0.0410156u |
| 同60fps相当 | 4800.08u、差0.0820312u |
| 同120fps相当 | 4799.84u、差0.164062u |
| 追いつきの数値シミュレーション | P10秒停止後の差1056.01uを約6.61667秒で解消。理論約6.60008秒。許容0.04秒内 |
| 終端・JSON | 終端超過抑止/到達後停止、0秒、変更した100u/sで2秒200u、必須キー欠損/不正比率/境界外拒否を確認 |
| 実画面・操作・シーン遷移 | 未実施。計算テストを実プレイ成功とは扱わない。下表のユーザー確認が必要 |
| x86 / Release | 既存設定で保留 / 未検証 |

60秒距離の許容差は2u（4800uの約0.042%）とし、float座標のフレームごとの丸めを含めて比較する。
計算テストは本番の変位関数を使うが、単独ではColliderManagerやObjectManager、入力、描画を通さない。
実Colliderテストは成功（終了0）。停止/遠方/重なりから追跡の3条件×30/60/120fpsで、拠点60秒の距離は各条件同一。誤差は順に0.0380859 / 0.0751953 / 0.150879uで許容2u内。Pコライダー破棄後も前進を確認。画面外というCamera状態そのものはこのテストに含めず、描画と更新が独立していることをコードで確認した。

## ユーザー確認

対象ビルド・確認者・日付・各項目の成功/失敗/未実施、失敗操作、表示座標を記録する。

| ID | 操作 | 期待 |
|---|---|---|
| B1 | 起動→Start | 青Pと橙BASE。P中心(512,2048)、BASE(768,2048)、80u/s。初期化後すぐ前進 |
| B2 | Pを操作せず10秒待つ | Pは停止、BASE中心Xが約1568へ進む。画面外になっても座標表示が増える |
| B3 | Pを左/上下へ離脱 | 拠点は+Xへ継続。移動速度はPの距離や画面内外で変わらない |
| B4 | 同じYで停止10秒後、右へ追跡 | 約6.6秒で追いつける。接触しても押し戻し・待機なし |
| B5 | R→Title→Startを2回 | 両者の初期位置・終端フラグがリセットされる |
| B6 | 約178秒待つ | BASE中心X15000でTitleへ戻る。正式な勝利画面ではない。再試行可能 |
| B7 | 元JSONをP200/B100へ変更し再起動、確認後240/80へ戻す | 表示・進行速度が変更値になる。変更前後、理由、結果を記録する。範囲外設定は試遊採用しない |

本環境ではネイティブ画面操作が利用できないためB1〜B7は未実施。JSONのアプリ内ロードから描画までの統合結果、実時間のFPS、再開始/Title復帰はテストの成功から推定しない。

## Issue #4からの残課題

PR #27はユーザー承認でマージ済みだが、U1〜U9と終了コード1は未解消。[フォローアップ#28](https://github.com/SyuLin1167/ProjectSurvivor/issues/28)へ移管した。
既存Execution::RunはGameLoop後に`_CrtDumpMemoryLeaks()`を返し、mainのExecution破棄はその後になる。これは静的確認であり、終了コード1の原因確定ではない。終了処理は本PRで変更しない。
#28の元U1は拠点なしの#4ビルドに対する手順。#5統合ビルドでは拠点の追加を期待し、U2〜U9はB1〜B7とあわせて回帰確認する。

#5のPRはユーザー検証待ちの間Draft/Blocked。#6以降へ進まず、ユーザー承認後のマージと結果を待つ。#7のPhase 1統合報告では#28を再確認する。

ログ: [本体ビルド](Verification/Issue5/build.txt)、[移動・JSONテスト](Verification/Issue5/tests.txt)、[実Colliderテストのビルド](Verification/Issue5/collider-build.txt)、[実Collider結果](Verification/Issue5/collider-tests.txt)。

## Issue #6: 方向・距離表示の操作確認

DEC-01の中央追従・固定ズーム・境界clampを維持し、画面外BASEを橙の矢印とBASE文字で表示。距離はPとBASEの中心間（u）。NEARは128u以内の測定目安であり、防衛範囲や勝敗条件ではない。詳細結果・対象SHAはIssue #6 / 対応PRを正本とする。

x64 Debug、初期JSON値で次を確認し、SHA・成功/失敗・問題箇所をIssue #6へ記録する。AIによるネイティブ操作・目視は未実施。

| ID | 操作 | 期待 |
|---|---|---|
| C1 | Start後にPを約20秒停止し、その後BASEを追う | 画面外でもBASEのX・距離が増加。矢印がBASEの方向を示し、追うと距離が減る。128u以内でNEAR、実体が画面に戻ると矢印が消える |
| C2 | Pを上下へ離脱し、BASEを追い越して左右関係も反転させる | 斜め・上下・左右の矢印が切れず、文字と距離が読める。相互通過しても表示が破綻しない |
| C3 | 外周と右端まで移動し、終端X15000まで試行する | 端ではカメラのみ停止しPは境界内。グリッド・P・BASEがずれず、表示が飛ばない。BASE到達でTitleへ戻る |
| C4 | 遠方からR→Startで再試行する | 古い追従先や矢印が残らず、初期位置と中心間距離256uへ戻る |

既存のRunMovementTests.cmdへ画面外矩形切替・長距離マーカーの回帰を追加。矩形の辺が接する場合も既存Rectの契約に合わせて表示対象とする。BaseColliderTestsでは実Cameraの境界・小マップclampを確認し、毎フレーム更新して、画面外の期間を含む30/60/120fps相当・60秒の拠点変位を比較する。専用テストプロジェクトや詳細ログは追加しない。
