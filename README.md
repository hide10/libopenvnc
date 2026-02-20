# libopenvnc

C言語で書かれたクロスプラットフォームVNC (RFBプロトコル) ライブラリ。

## 概要

libopenvncは、[RFC 6143](https://datatracker.ietf.org/doc/html/rfc6143)で定義されたRemote Framebuffer (RFB) プロトコルの実装です。クライアントとサーバの両機能を、MITライセンスで提供します。

### なぜ既存ライブラリを使わないのか

有名な[LibVNCServer/LibVNCClient](https://github.com/LibVNC/libvncserver)はGPL v2ライセンスのため、商用・プロプライエタリ利用に制約があります。libopenvncはMITライセンスで同等の機能を提供することを目指します。

## プロジェクト目標

- **RFC 6143完全準拠**: 仕様に定義された全機能をサポート
- **RFBバージョン**: 3.3, 3.7, 3.8
- **クライアント・サーバ両対応**: クライアントを優先的に実装し、サーバも提供
- **クロスプラットフォーム**: Windows優先、Linux等にも対応
- **寛容なライセンス**: MITライセンスによる自由な利用

## アーキテクチャ（設計予定）

```
┌─────────────────────────────┐
│  アプリケーション層          │  ← 録画アプリ、ビューア等（ライブラリ外）
├─────────────────────────────┤
│  クライアントAPI / サーバAPI │  ← 公開API
├─────────────────────────────┤
│  RFBプロトコルコア           │  ← エンコーディング、認証、メッセージ処理
├─────────────────────────────┤
│  トランスポート (TCP/TLS)    │  ← ネットワーク抽象化
├─────────────────────────────┤
│  プラットフォーム抽象化      │  ← OS固有コード (Win/Linux)
└─────────────────────────────┘
```

## RFC 6143 対応範囲

### エンコーディング
- Raw (0), CopyRect (1), RRE (2), Hextile (5), TRLE (15), ZRLE (16)
- 疑似エンコーディング: Cursor (-239), DesktopSize (-223)

### セキュリティタイプ
- None (1), VNC Authentication (2)

### クライアント→サーバ メッセージ
- SetPixelFormat, SetEncodings, FramebufferUpdateRequest, KeyEvent, PointerEvent, ClientCutText

### サーバ→クライアント メッセージ
- FramebufferUpdate, SetColourMapEntries, Bell, ServerCutText

## ロードマップ

1. **Phase 1**: RFC 6143仕様分析・詳細設計
2. **Phase 2**: コアプロトコル実装（ハンドシェイク、メッセージ型）
3. **Phase 3**: クライアント実装（接続、フレームバッファ受信、入力イベント）
4. **Phase 4**: エンコーディング実装（Raw → CopyRect → RRE → Hextile → TRLE → ZRLE）
5. **Phase 5**: セキュリティ（VNC Authentication）
6. **Phase 6**: サーバ実装
7. **Phase 7**: プラットフォーム固有の最適化とテスト

現在の状況: **Phase 1 - 完了、Phase 2準備中**

## ビルド

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

必要環境: CMake 3.15以上、C11対応コンパイラ

## ライセンス

MIT License。詳細は[LICENSE](LICENSE)を参照。
