# 初期調査・Phase計画

調査日: 2026-10-14 (Asia/Tokyo)。基準コミット: e8ec0b74c050c622f7e46ee1f7cfc1ef07cade17。

## 調査結果

- mainは2DGameSample基盤の移植状態。調査開始時Issues/PRはopen/closedとも0件。リンク済みProjectは0件、リンク候補もなし。新規管理Projectを作成。
- GitHubにはREADMEのプロジェクト名とsetup.batがあり、AGENTS.md・.githubの運用/CI設定・企画書はなかった。
- ローカルのDocs/GameDesign.md v0.1を企画基準として使用。このPRに元ファイルのバイト列を変更せず収録。
- ProjectName/codeにObject、Component、Collision、Asset、GameSystem、Scene、UI、include/MyLibに入力等の機能が存在する。
- Player.cppは横移動とJumpを追加し、既存Cameraへ追従対象を登録。Enemy.cppは静止コライダーと接触による自身のHP減少を持つ。Play.cppはPlayer/Enemy/Map/Goalを生成しResultへ遷移する。これらは再利用調査の手掛かりであり、MVP完成の証拠ではない。
- ローカルproject.vcxprojにはC++20および構成別のv145/v143設定がある。setup.batはDxLib取得、JSON取得、既存依存ディレクトリの削除を含む。実行せず内容確認に留めた。ビルド・起動・完全な更新順/寿命調査は #1/#2 で実施。
- mainのbranch情報ではprotected=false。マージの承認は運用ルールで管理する。保護設定の強制は今回の設定に含めない。

## 初期計画

Statusは初期計画。現在値はProjectを参照。依存はIssue本文に記録する。

| Issue | タスク | Phase | Priority | 初期Status | 依存 |
|---|---|---|---|---|---|
| #1 | P0-01 ビルド・起動手順と現状の動作を記録する | Phase 0 | P0 | Ready | — |
| #2 | P0-02 既存基盤の再利用箇所と不足機能を調査する | Phase 0 | P0 | Ready | — |
| #3 | DEC-01 Phase 1の試作条件を整理し企画担当の判断を得る | Phase 0 | P0 | Ready | — |
| #4 | P1-01 仮ステージと独立して移動できるプレイヤーを用意する | Phase 1 | P0 | Backlog | #1, #2, #3 |
| #5 | P1-02 プレイヤーを待たずに進む移動拠点を試作する | Phase 1 | P0 | Backlog | #4 |
| #6 | P1-03 カメラと画面外・長距離移動の挙動を整える | Phase 1 | P0 | Backlog | #5 |
| #7 | P1-04 離脱・追跡・再合流を検証しPhase 1を報告する | Phase 1 | P0 | Backlog | #6 |
| #8 | DEC-02 Phase 2の攻撃・標的・ダメージ条件の判断を得る | Phase 2 | P0 | Backlog | #2 |
| #9 | P2-01 基本敵の生成・破棄と標的への接近を実装する | Phase 2 | P0 | Backlog | #7, #8 |
| #10 | P2-02 プレイヤー攻撃1種類と敵の撃破を実装する | Phase 2 | P0 | Backlog | #9 |
| #11 | P2-03 敵の攻撃と拠点HP・ダメージ処理を実装する | Phase 2 | P0 | Backlog | #9 |
| #12 | P2-04 防衛戦闘の統合検証とPhase 2報告を行う | Phase 2 | P0 | Backlog | #10, #11 |
| #13 | DEC-03 Phase 3の開始・勝敗・HUD条件の判断を得る | Phase 3 | P0 | Backlog | #7 |
| #14 | P3-01 ゲーム開始と状態初期化を実装する | Phase 3 | P0 | Backlog | #12, #13 |
| #15 | P3-02 敵の継続出現を組み込む | Phase 3 | P0 | Backlog | #14 |
| #16 | P3-03 拠点進行とクリア・ゲームオーバーを実装する | Phase 3 | P0 | Backlog | #14 |
| #17 | P3-04 拠点HPと進行状況の最低限HUDを実装する | Phase 3 | P1 | Backlog | #16 |
| #18 | P3-05 開始から勝利・敗北までの通し検証を行う | Phase 3 | P0 | Backlog | #15, #17 |
| #19 | P4-01 速度・出現頻度の比較試遊と調整を行う | Phase 4 | P1 | Backlog | #18 |
| #20 | P4-02 離脱の利点・危険と再合流の体験を評価する | Phase 4 | P1 | Backlog | #19 |
| #21 | P4-03 不具合を整理しMVPの回帰検証を行う | Phase 4 | P1 | Backlog | #18 |
| #22 | P4-04 MVP評価と次の開発方針を報告する | Phase 4 | P1 | Backlog | #20, #21 |

Phase 0の開発成果は #1/#2、Phase 1着手の企画ゲートは #3。Phase 1〜4の完了判定はそれぞれ #7/#12/#18/#22。完了条件を満たす検証報告がない状態でPhaseを完了にしない。

## 最初の引き渡し

#1を取得してビルド・起動の基準を記録し、次に #2で既存機能と不足を調べる。#3の提案は調査結果を踏まえ企画担当に確認する。管理担当は実装を担当しない。企画条件が未決でも基盤調査は開始できる。
