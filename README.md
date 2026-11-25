# ProjectSurvivor

2DGameSample（C++20 Modules / DxLib）を基盤としたゲーム開発プロジェクトです。

攻略記録は Windows の `%LOCALAPPDATA%\ProjectSurvivor\campaign.json` に保存します。「続きから」は攻略状況を読み込んで依頼選択へ進みます（遠征途中の再開ではありません）。巡礼者の全2区間成功で魔導物資が解放されます。「最初から」は既存記録がある場合に確認を表示し、確定すると攻略状況を初期化します。破損・非対応版は自動上書きせず、保存失敗時は結果画面で再試行できます。

- 企画・運用: [GameDesign](Docs/GameDesign.md) / [DevelopmentWorkflow](Docs/DevelopmentWorkflow.md)
- ビルド・起動: [BuildBaseline](Docs/BuildBaseline.md)（MVP標準: x64 Debug、v143）
- 基盤の再利用調査: [FoundationAudit](Docs/FoundationAudit.md)
- 基盤の参照元・一致確認・環境差: [UpstreamReference](Docs/UpstreamReference.md)

基盤: [SyuLin1167/2DGameSample](https://github.com/SyuLin1167/2DGameSample)（既定ブランチ `develop-dxlib`）。[基盤ドキュメントサイト](https://syulin1167.github.io/2DGameSample/)は補助資料です。本プロジェクトの検証結果・MVP方針は上記Docsを参照してください。

基盤の元の著作権・MIT表示は[2DGameSampleの通知](Docs/ThirdParty/2DGameSample-LICENSE.md)に保存しています。画像・音声の出典や利用条件は基盤側でも未整理です。詳細は参照元の記録を確認してください。
