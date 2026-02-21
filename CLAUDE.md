# libopenvnc AI開発ガイド

このドキュメントは、本プロジェクトに関わるAIアシスタント向けの引き継ぎ資料です。

## プロジェクト概要

C言語によるVNC (RFBプロトコル) ライブラリ。MITライセンス。
オーナーの最終目的は、VNCクライアントを使ってリモート画面を録画すること。
ライブラリ自体に録画機能は含めない。録画はライブラリの上に別アプリケーションとして構築する。

## 決定済み事項

| 項目 | 決定 | 理由 |
|------|------|------|
| 言語 | C | AI駆動開発、最大限の互換性/FFI |
| ライセンス | MIT | LibVNCServerのGPLでは業務利用が困難 |
| ビルドシステム | CMake | クロスプラットフォームCライブラリの標準 |
| 準拠仕様 | RFC 6143 | RFBプロトコルの公式仕様 |
| スコープ | RFC 6143全体 | 仕様に定義された全エンコーディング、セキュリティタイプ、メッセージタイプ、RFB 3.3/3.7/3.8 |
| 優先度 | クライアント優先 | オーナーの直近ニーズがVNCクライアントによる画面録画 |
| サーバ | 実装する | 公開ライブラリのためサーバ機能も必要 |
| プラットフォーム | Windows優先 | ベース部分はクロスプラットフォーム（Linux等） |
| 録画機能 | ライブラリ外 | アプリケーション層の責務 |
| ドキュメント言語 | 日本語 | 英語版は別途用意しても良い |

## 現在のフェーズ

**Phase 2: コアプロトコル実装 — 完了** (PR #2でマージ済み)

### Phase 1の成果物 (完了)
1. `docs/rfb-protocol.md` — RFC 6143の詳細分析（全メッセージフォーマット、エンコーディング、プロトコルフロー）
2. `docs/rfb-version-differences.md` — RFB 3.3/3.7/3.8のバージョン差異と実装分岐ポイント
3. `docs/api-design.md` — 公開API設計（クライアント/サーバAPI、データ構造、モジュール構成、使用例）

### Phase 2の成果物 (完了)
- 静的ライブラリ `libovnc.a` のビルド（27ソースファイル）
- VNCクライアントがNone認証でサーバに接続し、Rawエンコーディングで画面更新を受信可能
- 18個のユニットテスト（全パス）
- サンプルクライアント `simple_client`

### Phase 2で実装したもの
- **プラットフォーム層**: POSIXソケット（connect/send/recv/close、タイムアウト対応）、Win32スタブ
- **トランスポート層**: TCP接続のラッパー（send_all/recv_all）
- **ハンドシェイク**: バージョンネゴシエーション（3.3/3.7/3.8）、セキュリティタイプ（None認証のみ）、ClientInit/ServerInit
- **メッセージ送受信**: 全10メッセージタイプ（C→S: 6種、S→C: 4種）
- **エンコーディング**: Raw、DesktopSize擬似エンコーディング、Cursor擬似エンコーディング
- **クライアントAPI**: 完全なライフサイクル管理（create/connect/run/disconnect/destroy）
- **サーバAPI**: スタブ（全関数がNULLまたはエラーを返す）

### Phase 2で未実装（後続フェーズで対応）
- VNC Authentication (DES暗号)
- TLS トランスポート
- CopyRect, RRE, Hextile, TRLE, ZRLE エンコーディング
- Win32ソケット実装（現在は `OVNC_ERR_NOT_IMPLEMENTED` を返すスタブ）
- サーバ機能の実装

**次のフェーズ: Phase 3**

## RFBプロトコル概要

- **RFB** = Remote Framebuffer Protocol（VNCが使用するプロトコル）
- **RFC 6143** = https://datatracker.ietf.org/doc/html/rfc6143
- RFBバージョン: 3.3（初期版）, 3.7（セキュリティネゴシエーション追加）, 3.8（エラーハンドリング改善、RFCで標準化）
- クライアントプルモデル: クライアントがフレームバッファ更新を要求する。サーバは自動プッシュしない
- ハンドシェイク: バージョンネゴシエーション → セキュリティタイプネゴシエーション → 認証 → 初期化

## 参考情報: LibVNCServer（既存GPLライブラリ）

GPL v2ライセンスのため採用を見送り。ただし設計上の知見は参考になる:
- コールバックベースのAPI設計が有効（MallocFrameBuffer, GotFrameBufferUpdate, FinishedFrameBufferUpdate）
- `client->frameBuffer` への直接ポインタアクセスパターンは録画に効率的
- `vnc2mpg.c` というVNC→MP4録画サンプルが存在（FFmpeg連携）
- RFBはプル型: 録画時は `SendFramebufferUpdateRequest` を継続的に送信する必要がある

## アーキテクチャ設計

```
┌─────────────────────────────┐
│  アプリケーション層          │  ← ライブラリ外
├─────────────────────────────┤
│  クライアントAPI / サーバAPI │  ← 公開API (ovnc_client.h / ovnc_server.h)
├─────────────────────────────┤
│  RFBプロトコルコア           │  ← エンコーディング、認証、メッセージ解析
├─────────────────────────────┤
│  トランスポート (TCP/TLS)    │  ← ネットワークI/O抽象化
├─────────────────────────────┤
│  プラットフォーム抽象化      │  ← Win32/POSIXソケット、スレッド等
└─────────────────────────────┘
```

## コード規約

- C11標準
- 命名規則: `ovnc_` プレフィクス（詳細は `docs/api-design.md` 参照）
  - 公開API: `ovnc_client_*`, `ovnc_server_*`, `ovnc_*`
  - 内部関数: `ovnc__*` (ダブルアンダースコア)
  - 型: `ovnc_*_t` (構造体), `OVNC_*` (列挙値/マクロ)
  - コールバック: `ovnc_*_fn`
- エラーハンドリング: `ovnc_error_t` 列挙型を返す。成功は `OVNC_OK` (0)

## 現在のファイル構成

```
libopenvnc/
├── .gitignore
├── CMakeLists.txt              ← ビルド設定（静的ライブラリ + テスト + サンプル）
├── LICENSE                     (MIT)
├── README.md                   (プロジェクト概要・ロードマップ)
├── CLAUDE.md                   (このファイル)
├── cmake/
│   └── ovnc_version.h.in      ← バージョンテンプレート
├── docs/
│   ├── rfb-protocol.md         (RFC 6143 詳細仕様ノート)
│   ├── rfb-version-differences.md (バージョン差異)
│   └── api-design.md           (公開API設計)
├── include/ovnc/
│   ├── ovnc.h                  ← アンブレラヘッダ
│   ├── ovnc_common.h           ← 共通型・エラーコード・構造体
│   ├── ovnc_client.h           ← クライアントAPI宣言
│   └── ovnc_server.h           ← サーバAPI宣言（スタブ）
├── src/
│   ├── ovnc_internal.h         ← 内部構造体・バイトオーダーヘルパー
│   ├── client/
│   │   └── ovnc_client.c       ← クライアントライフサイクル実装
│   ├── common/
│   │   ├── ovnc_error.c        ← エラー文字列
│   │   └── ovnc_pixel.c        ← ピクセルフォーマットのシリアライズ/デフォルト
│   ├── platform/
│   │   ├── ovnc_platform.h     ← プラットフォーム抽象化ヘッダ
│   │   ├── ovnc_socket_posix.c ← POSIXソケット実装
│   │   └── ovnc_socket_win32.c ← Win32スタブ
│   ├── protocol/
│   │   ├── ovnc_encoding.h     ← エンコーディング内部ヘッダ
│   │   ├── ovnc_encoding.c     ← エンコーディングディスパッチャ
│   │   ├── ovnc_enc_raw.c      ← Rawエンコーディングデコーダ
│   │   ├── ovnc_handshake.h    ← ハンドシェイク内部ヘッダ
│   │   ├── ovnc_handshake.c    ← バージョン/セキュリティ/初期化
│   │   ├── ovnc_messages.h     ← メッセージ送受信内部ヘッダ
│   │   └── ovnc_messages.c     ← 全10メッセージタイプの送受信
│   ├── server/
│   │   └── ovnc_server.c       ← サーバスタブ
│   └── transport/
│       ├── ovnc_transport.h    ← トランスポートインターフェース
│       └── ovnc_transport.c    ← TCP トランスポート実装
├── examples/
│   ├── CMakeLists.txt
│   └── simple_client.c         ← 最小VNCクライアント
└── tests/
    ├── CMakeLists.txt
    └── test_protocol.c         ← ユニットテスト (18テスト)
```

## ビルド方法

```bash
cmake -B build
cmake --build build
# テスト実行
ctest --test-dir build
# またはテストバイナリ直接実行
./build/tests/test_protocol
```

必要環境: CMake 3.15以上、C11対応コンパイラ

## 実装上の注意点

### Phase 2レビューで確立されたパターン
- **RFB 3.3のVNC Auth**: Phase 2では未実装。3.3でVNC Auth(タイプ2)が返された場合、`OVNC_ERR_AUTH_UNSUPPORTED` で即座にエラーを返す（DESストリームの読み飛ばしを試みない）
- **サーバ名長すぎ**: ServerInitでサーバ名が `OVNC_MAX_SERVER_NAME_LEN` (1024) を超える場合、固定256バイトバッファでチャンク読み飛ばし（mallocしない）
- **ピクセルフォーマット検証**: `bits_per_pixel` は 8/16/32 のみ許可。他の値は `-1` を返す
- **SetPixelFormat後のフレームバッファ再割り当て**: bppが変わった場合、`data_size` を再計算して再割り当てする
- **プラットフォーム参照カウント**: `ovnc__platform_init/cleanup` は参照カウント方式。Win32の `WSAStartup/WSACleanup` 対称呼び出しを保証
- **Win32スタブ**: ソケット関数は `OVNC_ERR_NOT_IMPLEMENTED` を返す（`OVNC_ERR_IO` と区別）
- **freeaddrinfo安全性**: POSIXソケットの `connect` で `goto cleanup` パターンを使い、必ず `freeaddrinfo` を呼ぶ

## 次のステップ

1. ~~RFC 6143を精読・分析する~~ ✓
2. ~~docs/ ディレクトリに仕様ノートを作成する~~ ✓
3. ~~公開APIを設計する~~ ✓
4. ~~Phase 1ドキュメントをコミットする~~ ✓
5. ~~Phase 2: コアプロトコル実装~~ ✓
6. Phase 3: 追加エンコーディング実装
   - CopyRect, RRE, Hextile, TRLE, ZRLE
7. Phase 4: VNC Authentication (DES暗号)
8. Phase 5: Win32ソケット実装
9. Phase 6: サーバ実装
10. Phase 7: TLS トランスポート
