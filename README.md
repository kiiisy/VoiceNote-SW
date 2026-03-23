# VoiceNote-SW

VoiceNote のソフトウェア開発用リポジトリです。\
Zynq AMP 構成で、Core0（音声処理）と Core1（UI）を分離して実装しています。

※ まだ設計書は用意できていないです。途中なので出来次第登録します。

> [!WARNING]
> 個人開発ベースのため、すべての環境での動作は保証していません。\
> とくに Vitis / Vivado のバージョン差分やローカルパス依存に注意してください。

## フォルダ構成

```text
.
├── README.md                       # このリポジトリの入口ドキュメント
├── from_hw/
│   └── design_1_wrapper.xsa        # HWエクスポート
├── voice_note_app_amp0/            # Core0アプリ（音声処理/IPCサーバ）
│   ├── src/
│   └── build/
├── voice_note_app_amp1/            # Core1アプリ（UI/IPCクライアント）
│   ├── src/
│   └── build/
├── liblvgl/                        # LVGLライブラリ
│   └── src/
├── voice_note_platform/            # プラットフォーム/BSP/FSBL資産
│   ├── hw/
│   ├── ps7_cortexa9_0/
│   ├── ps7_cortexa9_1/
│   └── zynq_fsbl/
└── voice_note_sys/                 # 統合管理
    ├── doc/
    ├── build/
    └── output/
```

## はじめかた

メンテする予定

## Push / PR ルール

GitHub Actionsを利用しているため、`main` へは直接pushせず、作業ブランチ経由でPull Requestを作成してください。

### 基本ルール

1. `main` から作業ブランチを作成する
2. 作業ブランチに commit / push する
3. Pull Request を作成する
4. GitHub Actions が成功してからレビュー・マージする
5. `main` への直接 push は行わない

### 推奨手順

```bash
# main を最新化
git checkout main
git pull origin main

# 作業ブランチ作成
git checkout -b dev-tmp

# 変更をコミット
git add .
git commit -m "chore: add push and PR workflow"

# リモートへ push
git push -u origin dev-tmp
```
