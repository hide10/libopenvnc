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

## 現在の実装状況

### 実装済み
- **クライアントAPI**: 接続、フレームバッファ受信、入力イベント送信、切断のフルライフサイクル
- **RFBハンドシェイク**: バージョンネゴシエーション (3.3/3.7/3.8)、セキュリティタイプ (None)
- **メッセージ送受信**: RFC 6143で定義された全10メッセージタイプ
- **エンコーディング**: Raw, DesktopSize (擬似), Cursor (擬似)
- **プラットフォーム**: POSIX ソケット (Linux/macOS)
- **テスト**: 18個のユニットテスト

### 未実装
- VNC Authentication (DES暗号)
- CopyRect, RRE, Hextile, TRLE, ZRLE エンコーディング
- Win32 ソケット (スタブのみ)
- TLS トランスポート
- サーバ機能 (スタブのみ)

## アーキテクチャ

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
- **実装済み**: Raw (0), DesktopSize (-223), Cursor (-239)
- **未実装**: CopyRect (1), RRE (2), Hextile (5), TRLE (15), ZRLE (16)

### セキュリティタイプ
- **実装済み**: None (1)
- **未実装**: VNC Authentication (2)

### クライアント→サーバ メッセージ (全6種 実装済み)
- SetPixelFormat, SetEncodings, FramebufferUpdateRequest, KeyEvent, PointerEvent, ClientCutText

### サーバ→クライアント メッセージ (全4種 実装済み)
- FramebufferUpdate, SetColourMapEntries, Bell, ServerCutText

## ビルド

```bash
cmake -B build
cmake --build build
```

必要環境: CMake 3.15以上、C11対応コンパイラ

### テスト実行

```bash
ctest --test-dir build
```

### サンプルクライアント

```bash
# None認証のVNCサーバに接続
./build/examples/simple_client <host> [port]
```

## 使用例

```c
#include <ovnc/ovnc.h>

static void on_fb_update(ovnc_client_t *client, const ovnc_rect_t *rect) {
    printf("Updated: %dx%d at (%d,%d)\n",
           rect->width, rect->height, rect->x, rect->y);
}

int main(void) {
    ovnc_client_config_t config = {0};
    config.host = "192.168.1.100";
    config.port = 5900;

    ovnc_client_callbacks_t cbs = {0};
    cbs.framebuffer_update = on_fb_update;

    ovnc_client_t *client = ovnc_client_create(&config, &cbs);
    ovnc_error_t err = ovnc_client_connect(client);
    if (err == OVNC_OK) {
        ovnc_client_run(client);  /* メッセージループ */
    }
    ovnc_client_destroy(client);
    return 0;
}
```

## ロードマップ

1. **Phase 1**: RFC 6143仕様分析・詳細設計 — **完了**
2. **Phase 2**: コアプロトコル実装（ハンドシェイク、メッセージ、Rawエンコーディング、クライアントAPI） — **完了**
3. **Phase 3**: 追加エンコーディング（CopyRect, RRE, Hextile, TRLE, ZRLE）
4. **Phase 4**: セキュリティ（VNC Authentication）
5. **Phase 5**: Win32ソケット実装
6. **Phase 6**: サーバ実装
7. **Phase 7**: TLS トランスポート・最適化

## ライセンス

MIT License。詳細は[LICENSE](LICENSE)を参照。
