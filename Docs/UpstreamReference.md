# 2DGameSample参照元と差異の確認

確認日: 2026-10-17 JST。ユーザーから提示された基盤リポジトリを、Issue #2 / PR #26の追加調査として照合した。これは基盤更新の取り込みや仕様変更ではない。

## 比較対象と方法

| 対象 | 固定した基準 |
|---|---|
| 基盤リポジトリ | [SyuLin1167/2DGameSample](https://github.com/SyuLin1167/2DGameSample) |
| 基盤の既定ブランチ | `develop-dxlib`（origin/HEADで確認。mainを比較対象にしない） |
| 基盤の今回取得HEAD | [00aa2c4482bfea9319163ea9c6e5c6b4e3a6ef58](https://github.com/SyuLin1167/2DGameSample/commit/00aa2c4482bfea9319163ea9c6e5c6b4e3a6ef58) |
| 基盤文書自身のコード確認基準 | `08f9fa53fa5e9043d46aeede8e7aab7bab519892`。上記HEADまでのProjectName/setup.batに差分なし |
| ProjectSurvivor | main `86fce548461ac8c2b5c72e81cedecf6c1196bd1f`（PR #25マージ後） |
| 調査報告の初回成果 | `e9fb05435b5a83f0d67c3f488f995f1a0d9340a9`、Docs/FoundationAudit.mdのみ追加 |

別ディレクトリーへ基盤をcloneし、両リポジトリの`git ls-tree -r <commit>`でパス集合とblob IDを照合した。作業コピーの改行変換・ダウンロード依存物・生成物に左右されない、追跡済みファイルの比較である。

## 差異の結果

| 範囲 | 結果・扱い |
|---|---|
| ProjectName配下 | **共通133ファイルはすべて同一blob**。C++コード、MyLib、sln/vcxproj、設定・素材に差異なし。基盤側134件に対しProjectSurvivor側133件で、基盤のみの1件は生成ログ`ProjectName/project/Log.txt`。実装差ではなく、ログは移植せず今回の実行証拠にも扱わない |
| setup.bat | 同一blob。DxLib移動失敗の検出不足、JSON develop取得の非固定性も共通。基盤側のsetupへ置換して解決する差ではない |
| 共通パス全体 | 一致134、差異3（README.md、LICENSE、.gitignore） |
| README | ProjectSurvivorは名称のみだった。基盤の所在、ビルド記録・再利用調査への導線を本PRで補足 |
| LICENSE | 基盤は2025 SyuLin1167、ProjectSurvivorは2026 SyuLin1167。元のMIT通知を[別文書](ThirdParty/2DGameSample-LICENSE.md)に保存し、ルートLICENSEは維持 |
| .gitignore | 一般的なVSテンプレートにも差があり、ProjectSurvivorには取得したDxLib/nlohmannの除外がなかった。本PRでこの2ディレクトリーだけ除外。テンプレート全体は置換しない |
| 基盤のみの文書/設定 | 小文字`docs/`のガイド・API・チュートリアル・VitePress設定、docs-check/docs-deployのActions、ルート.gitattributes。ProjectSurvivorへ一括導入しない |
| ProjectSurvivor固有 | AGENTS、Issue/PRテンプレート、企画・運用・計画・仕様判断・ビルド検証の`Docs/`。基盤文書はこれらの仕様承認を代替しない |

今回の一致は**指定コミット間の内容一致**であり、最初に取り込んだ基盤コミットの履歴上の同定までは行っていない。基盤はsubmoduleではなく、ProjectSurvivor内の通常ファイルとして管理されている。今後の基盤HEADの変化は自動反映されない。

## 既存報告との照合

更新順、シーン/オブジェクトの所有、Componentの順序非保証、動的追加時の参照初期化差、カテゴリ単位のCollider削除、複合マスクの完全一致検索は、基盤の[lifecycle](https://github.com/SyuLin1167/2DGameSample/blob/00aa2c4482bfea9319163ea9c6e5c6b4e3a6ef58/docs/guide/lifecycle.md)、[ownership](https://github.com/SyuLin1167/2DGameSample/blob/00aa2c4482bfea9319163ea9c6e5c6b4e3a6ef58/docs/architecture/ownership.md)、[status](https://github.com/SyuLin1167/2DGameSample/blob/00aa2c4482bfea9319163ea9c6e5c6b4e3a6ef58/docs/project/status.md)と整合する。FoundationAuditのR1～R8は引き続き有効。基盤側も静的確認と実機検証を区別している。

基盤READMEのDebug/Release x64推奨はビルド成功の証拠ではない。[基盤の検証記録](https://github.com/SyuLin1167/2DGameSample/blob/00aa2c4482bfea9319163ea9c6e5c6b4e3a6ef58/docs/project/verification.md)ではC++ビルド未実施、既存Releaseの撮影もソース一致未確認としている。ProjectSurvivorのx64 Debug成功は[独自の検証記録](BuildBaseline.md)に基づく。Release未検証・x86保留のMVP方針は変更しない。

## 追加で明記する制約

基盤文書の記述を同一のローカルソースと照合した。以下の結果は静的確認で、実行再現済みの一覧ではない。

| 項目 | 確認した実装と利用条件 | 後続の扱い |
|---|---|---|
| 円対タイル・多角形 | [TileColliderVisitor](../ProjectName/code/Collision/TileCollider/TileColliderVisitor.ixx)の円Visitと[TileCollider](../ProjectName/code/Collision/TileCollider/TileCollider.ixx)のAcceptは空。POLYの対応クラスなし | 円の敵/拠点が地形と衝突すると仮定しない。#5/#9で形状・地形要件を照合。矩形対タイルは地形側から訪問する |
| IDの管理範囲 | ColliderID/UIElementIDはindexとgenerationのみでManager識別子なし | シーンを越えて使い回さない。#9/#14の破棄・再開始検証へ |
| アニメーション | [Animator](../ProjectName/code/Component/Animator/Animator.cpp)にONCE停止分岐なし、PINGPONGでendFrameへ設定。EnableAnimは描画スキップ側のフラグを立てる | #4/#10でアニメ追加時に最終添字・単発停止・表示切替を検証。LOOPの主要操作確認を全方式へ一般化しない |
| アニメ定義の取得 | [Animation::GetAnim](../ProjectName/code/Asset/Animation/Animation.ixx)は値返却、Animator側で参照束縛する箇所あり | 基盤文書もコンパイル・寿命の確認事項としている。#1のx64 Debugビルドは成功しているので「ビルド不能」と断定しない。利用箇所変更時に式と寿命を再確認 |
| ロード再訪 | [LoadingScene](../ProjectName/code/Scene/LoadingScene/LoadingScene.cpp)のelapsedはstaticでリセットなし | 再訪時の0.5秒待ちを保証しない。#14で繰返し遷移を確認。Loading同一インスタンスのRun再実行も前提にしない |
| BGM | [Sound](../ProjectName/code/Asset/Sound/Sound.cpp)はLoadBGM経路を持つが、[AudioSource](../ProjectName/code/Component/AudioSource/AudioSource.cpp)の単発分岐はSE_ONCEのみ、他はSoundMemのループ系 | 歩行SEの存在をBGM対応保証にしない。音声拡張時にAPI系統・停止/削除・単発を個別検証 |
| JSON/CSV | [JsonIO::Write](../ProjectName/include/MyLib/FileSystem/FileIO/JsonIO.ixx)の拡張子補完は一時文字列に対する加算。[CsvIO](../ProjectName/include/MyLib/FileSystem/FileIO/CsvIO.ixx)は空セルを省略 | 書出し時は明示的な.json、マップCSVは空セルなしの数値列を使用。#4のデータ変更で寸法・セル数を検査。一般的な引用符付きCSV対応とは扱わない |
| 外部バックエンド | 現ブランチはDxLib/Windows。SDL/ImGui/FMOD導入なし | 不足依存として追加インストールしない |

## 必要な環境・設定と今回の対応

- Windows、VSの「C++によるデスクトップ開発」、**v143**、Windows SDKが必要。VS2026でもv145への変更は不要。プロジェクトSDK指定は10.0、#1で使用した版は10.0.26100.0。
- MVP検証はDebug/x64。DxLib 3.24fとJSONヘッダーを配置し、[BuildBaseline](BuildBaseline.md)の取得版/ハッシュと起動配置を参照する。既存の復旧済みコピーでsetup.batを再実行する必要はない。
- `.gitignore`へ`/ProjectName/include/DxLib/`と`/ProjectName/include/nlohmann/`を追加した。取得依存物の誤コミット防止で、ビルド設定や配布条件の変更ではない。既に追跡したファイルを削除する処理は行わない。
- 元の小文字`docs/`をWindows上の既存`Docs/`へそのままコピーしない。ケース非区別の環境では衝突するため、固定コミットのリンクを参照する。VitePress/Node.js・サイト用Actionsはゲーム実行に不要で、導入しない。
- upstream remote、submodule、自動同期は必須でなく、今回は設定しない。基盤更新は対象SHAと差分を固定し、Issue単位で検証して取り込む。

## 出典・素材情報の残件

[基盤credits](https://github.com/SyuLin1167/2DGameSample/blob/00aa2c4482bfea9319163ea9c6e5c6b4e3a6ef58/docs/project/credits.md)では画像・音声の作者/出典/利用・再配布・動画掲載条件が未整理。今回のコード一致からこれらの条件を確定しない。基盤の2025 MIT表示はそのまま別文書に保存した。

残る情報: Title/Play/Result/bg/map/playerの画像、BGM_TestLoop、SE_Confirm/Footsteps/Jumpについて、作者・配布URL・ライセンス・加工・配布/掲載条件。情報提供窓口はユーザー/素材管理者。DxLibとJSONの通知は実際に採用する配布物・版で記録する。今回、素材の削除/置換や利用条件の推測による確定は行わない。

## 再確認手順と検証範囲

1. 基盤を別の比較専用コピーへ取得し`git symbolic-ref refs/remotes/origin/HEAD`と`git rev-parse HEAD`を記録。
2. 両リポジトリで`git ls-tree -r <固定SHA>`を取り、パスとblob IDを比較。片側だけのファイルも別記する。
3. 基盤文書基準から取得HEADまでの`git diff <文書基準> <取得HEAD> -- ProjectName setup.bat`を確認。
4. 必要な構成はBuildBaselineとvcxprojを照合。文書の推奨と実行確認を区別する。
5. 本PRのリンク/差分と`git check-ignore`で2依存ディレクトリーを確認。ゲームコード・ビルド設定・setup.batへの差分がないことを確認。

今回はコード・設定の内容比較と文書照合を実施した。新しいビルド/ゲーム操作は未実施（ゲームコード・ビルド入力を変更しておらず、#1の記録を参照）。特殊条件は上表の後続検証へ残す。基盤側リポジトリには変更を加えていない。
