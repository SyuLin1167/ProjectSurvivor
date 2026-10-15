# Phase 0: ビルド・起動の基準調査（Issue #1）

調査日: 2026-10-15 (Asia/Tokyo)。対象: main `cbdc755a6f8e21033cdd2641939efb4d2ff3512b`。
作業ブランチ: `issue/1-baseline`。状態: **Blocked / 調査成果提出、完了ではない**。
解消先: [#24](https://github.com/SyuLin1167/ProjectSurvivor/issues/24)。既存ソース、プロジェクト設定、setup.batは変更していない。

## 結果

| 検証 | 期待 | 実測 |
|---|---|---|
| Debug x64 / v143 | exe生成 | 成功、終了コード0、64警告・0エラー |
| Debug x86 / v145 | exe生成 | 失敗、終了コード1、LNK1104: box2d.lib不足 |
| Debug x64起動 | ウィンドウ・DxLib初期化 | 8秒後に`2DGame`、Responding=True、HasExited=False。DxLib/Direct3D11初期化完了 |
| リソース配置 | exe横へコピー | `ProjectName/x64/Debug/resources`へ13ファイル配置 |
| Start・移動・ジャンプ・Goal・Result・終了 | 実操作で確認 | 未実施。本セッションではネイティブ画面操作・撮影が無効。静的確認は後述 |
| Release x64/x86 | 構成別検証 | 未実施。Debugを基準に調査し、x86障害と操作検証不足を先に記録。Releaseの成功は主張しない |

初回のサンドボックス内ビルドはx64/x86ともC1902（PDBマネージャー不一致）で失敗した。同一構成を制限外で再実行すると通過したため、初回失敗はこの実行環境の制限に関係すると判断する。ツールセット変更やPDB設定変更は行っていない。

起動ログ末尾には約9秒後のDxLib終了処理と割当0の記録があるが、操作による終了は検証していない。コマンド実行終了時のプロセス寿命の影響を切り分けておらず、正常なExit操作の証拠には使わない。タイトル画像の目視、音、ゲームプレイ、終了コードは未確認。

## 環境・構成

- Windows 11 25H2、build 26200.9457、x64。DxLibログもWindows11/build 26200を報告（レジストリProductNameの旧表記はWindows 10 Home）。
- Visual Studio Community 2026 18.10.3 (`18.10.12224.181`)。
- MSBuild `18.10.1.42706`。場所: `C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe`。
- v143: MSVC tools `14.44.35207`、cl `19.44.35229`。v145: tools `14.51.36231`、cl `19.51.36260`。
- Windows SDK `10.0.26100.0`をビルドログで確認。インストール済みには`10.0.22621.0`もある。
- `ProjectName/2DGameSample.sln`はVisual Studio Version 17形式。単一C++プロジェクト`ProjectName/project/project.vcxproj`。全構成C++20、MultiByte、Windowsサブシステム、C++モジュール依存スキャンあり。

| ソリューション構成 | プロジェクト構成 | Toolset | CRT | 追加リンク依存 |
|---|---|---|---|---|
| Debug x64 | Debug x64 | v143 | /MTd | Box2D指定なし |
| Release x64 | Release x64 | v143 | /MT | Box2D指定なし |
| Debug x86 | Debug Win32 | v145 | /MTd | box2d.lib |
| Release x86 | Release Win32 | v145 | /MT | box2d.lib |

SDK指定は`10.0`で特定版固定ではない。今回解決された版を上記に記録した。

## 依存の導入と再現性

クリーンcloneには`ProjectName/include/MyLib`のみ存在した。DxLib、nlohmann、Box2Dは未同梱。MyLibはリポジトリ内コードであり別途導入不要。

- DxLib: setup.bat指定の `https://dxlib.xsrv.jp/DxLib/DxLib_VC3_24f.zip`。ヘッダーの`DXLIB_VERSION_STR`も3.24f。
- JSON: setup.batはnlohmann/jsonのdevelopから`json.hpp`と`json_fwd.hpp`を取得するため版が変動する。今回はdevelop HEAD `4e294f4b181435c89cc0fceb21ff2b5941d96fdc`を解決し、そのコミットの`single_include/nlohmann/`から両方取得。マクロ版は3.12.0（正式タグ版と同一とは主張しない）。
- Box2D: Win32構成の`AdditionalDependencies`にのみ`box2d.lib`。導入手順・配布物なし。`code`/`include/MyLib`の検索ではBox2D参照を検出しなかったが、不要と断定して設定削除は行っていない。

| 取得物 | SHA-256 |
|---|---|
| DxLib_VC3_24f.zip | `82fde04562e4a5128710bb95ad8a73dca708a9e080439acc14d8e1ad5a095675` |
| json.hpp | `16a9185f661eadb85cde13b5b995e9c54f92c71044a743dc6d0e1b4346666af8` |
| json_fwd.hpp | `6484b6b4ac0b135d9662ca747956b6a465c21ffcda4b4bfd2e5f03f2e2cc0cf0` |

### setup.batの扱い

実行前に全内容を読んだ。カレントディレクトリを基準に`dxlib_tmp`の再帰削除、JSONフォルダーの削除・再作成、ZIPの削除、最後にsln起動を行う。DxLib移動の失敗チェックがなく、JSONの最初の取得失敗が後の成功で隠れる可能性もある。既存依存のある作業場所で無条件実行しない。

今回バッチは未実行。新規clone内の`.baseline`にZIPを保存・展開し、DxLib.hを含む配布フォルダー全体を、新規の`ProjectName/include/DxLib`へコピーした。PowerShellのExpand-Archiveで日本語フォルダー名が文字化けし、バッチ記載の日本語パスを指定した最初のコピーは失敗した。DxLib.hが1件であることを確認して親フォルダーをコピーし、解消した。依存ファイルや一時ZIPはコミット対象にしない。

再現手順（リポジトリルート、PowerShell、新規clone）:

```powershell
git clone --branch main --single-branch https://github.com/SyuLin1167/ProjectSurvivor.git ProjectSurvivor
Set-Location ProjectSurvivor
git switch --detach cbdc755a6f8e21033cdd2641939efb4d2ff3512b
New-Item -ItemType Directory .baseline
curl.exe -fL https://dxlib.xsrv.jp/DxLib/DxLib_VC3_24f.zip -o .baseline/DxLib_VC3_24f.zip
Expand-Archive -LiteralPath .baseline/DxLib_VC3_24f.zip -DestinationPath .baseline/dxlib
$dxHeader = Get-ChildItem .baseline/dxlib -Filter DxLib.h -Recurse
if (@($dxHeader).Count -ne 1) { throw 'Expected exactly one DxLib.h' }
Copy-Item -LiteralPath $dxHeader.Directory.FullName -Destination ProjectName/include/DxLib -Recurse
New-Item -ItemType Directory ProjectName/include/nlohmann
curl.exe -fL https://raw.githubusercontent.com/nlohmann/json/4e294f4b181435c89cc0fceb21ff2b5941d96fdc/single_include/nlohmann/json.hpp -o ProjectName/include/nlohmann/json.hpp
curl.exe -fL https://raw.githubusercontent.com/nlohmann/json/4e294f4b181435c89cc0fceb21ff2b5941d96fdc/single_include/nlohmann/json_fwd.hpp -o ProjectName/include/nlohmann/json_fwd.hpp
Get-FileHash .baseline/DxLib_VC3_24f.zip,ProjectName/include/nlohmann/json.hpp,ProjectName/include/nlohmann/json_fwd.hpp
```

各curlの終了コード0と上記ハッシュを確認してからビルドする。既存フォルダーへの上書きや削除はこの手順に含めない。

## ビルド・起動手順

VSのC++デスクトップ開発環境と上記toolset/SDKを用意し、slnを開いてDebug/x64を選ぶ。またはリポジトリルートで実行する:

```powershell
$msbuild = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
& $msbuild ProjectName/2DGameSample.sln /t:Build /p:Configuration=Debug /p:Platform=x64 /nologo /v:minimal /fl '/flp:logfile=.baseline/build-debug-x64.log;verbosity=normal'
# x86の失敗再現は /p:Platform=x86 に変更
$exe = (Resolve-Path ProjectName/x64/Debug/project.exe).Path
Start-Process -FilePath $exe -WorkingDirectory (Split-Path $exe)
```

今回のビルドは構成を上書きせずslnから行った。x64の出力は`ProjectName/x64/Debug/project.exe`。起動処理はexeの`../../resources`（この配置なら`ProjectName/resources`）をexe横の`resources`へコピーする。exeだけを別の場所へ移すと同じ前提にならない。既存のexe横リソースは起動時に上書きされる。x86の通常出力配置ではこの相対参照が異なるため、ビルド解消後に別途確認が必要。

## 初期状態と操作（ソースからの確認、プレイ実測ではない）

- 初期シーンはTitle。1920×1080 / 32bitのウィンドウ、タイトル`2DGame`、Direct3D11。Title.pngとStart/Exitボタンを配置。
- Startは左クリックをボタン内で押して離す。Title.cppの「Eキー」コメントと異なり実装はボタン経由。ロード画面からPlayへ移動。
- PlayはPlayer、Enemy、Map、Goalを各1つ生成。プレイヤー位置(100,150)、サイズ32×64、水平速度150、ジャンプ速度500がPlayerData.jsonの初期値。
- 左右矢印で水平移動、Spaceの押下でジャンプ。カメラはプレイヤー追従。歩行アニメーション・歩行音の処理がある。
- Enemyは(480,800)の静止円コライダー。Playerとの接触で敵自身のHPが減る。
- Goalは(2100,1000)のトリガー。Player接触でResultへ。RでTitleへ戻り、Exitボタンで終了要求。
- これは既存2DGameSampleの横移動サンプルの状態であり、移動拠点防衛や8方向移動の完成・仕様承認を意味しない。

手動検証時は、タイトルの目視→Start→左右移動→Space→敵接触→Goal→Result→R→Exitを順に確認し、画像、操作、期待/実際、終了コード、DxLibログを追記する。開始できない場合も失敗地点を記録する。

## 証拠と残課題

`Docs/Verification/Issue1/`にビルド出力と起動記録を収録。公開ログの作業コピー絶対パスは`<REPO>`へ正規化し、DxLibログのハードウェア詳細は省略した。元の完全ログはローカル`.baseline`とexe横のLog.txtに残す。

- Debug x64: `build-debug-x64.txt`（制限外での成功ログ）
- Debug x86: `build-debug-x86.txt`（制限外でのLNK1104ログ）
- 初回環境エラー: `sandbox-pdb-errors.txt`
- 起動観測: `runtime.txt`

警告C4819（コードページ932）、C4244、C4018等は未修正。Release、画面の見え方、操作、音、長時間実行、Exit正常終了は未検証。#24でx86依存/対象構成の判断と操作検証環境を解消し、#1を再検証する。担当窓口はユーザー（対象構成・手動検証）、開発担当AI（承認範囲の検証）。次回確認は判断回答または操作検証環境が利用可能になった時。

Issue #1の環境・手順の記録は実施済みだが、全体を完了扱いにしない。文書はDraft PRで管理し、ユーザー承認なしのマージ・Close・Doneは行わない。Issue #2以降とPhase 0全体の完了判定は今回の対象外。
