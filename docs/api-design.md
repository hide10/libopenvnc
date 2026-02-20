# libopenvnc API設計

本ドキュメントはlibopenvncライブラリの公開API、モジュール構成、データ構造の詳細設計である。

---

## 1. 設計原則

1. **C11標準準拠**: 移植性のためC11標準のみを使用
2. **コールバックベースAPI**: 非同期イベントをコールバック関数で通知
3. **メモリ管理の明確化**: 所有権のルールを明確にし、ユーザが制御可能に
4. **スレッド安全性**: メッセージ送信はスレッドセーフ。コールバックはI/Oスレッドから呼ばれる
   - `ovnc_client_get_framebuffer()` が返すポインタはコールバック外からも参照可能だが、
     コールバック実行中にライブラリがフレームバッファを更新するため、コールバック外から
     読み取る場合は利用者側でロックを取る必要がある
   - コールバック内（`framebuffer_update`, `framebuffer_update_finished`）では
     フレームバッファの該当矩形は更新済みであり、安全に読み取れる
   - `alloc_framebuffer` コールバック後にポインタが変わる可能性がある（再確保時）。
     古いポインタをキャッシュしている場合は無効になる
5. **エラー処理**: 戻り値によるエラー通知 + エラーコード取得関数
6. **最小依存**: 外部依存はzlib（ZRLE用）のみ必須。TLS/暗号はオプション

---

## 2. 命名規則

### プレフィクス

| スコープ | プレフィクス | 例 |
|----------|-------------|-----|
| 公開API (クライアント) | `ovnc_client_` | `ovnc_client_connect()` |
| 公開API (サーバ) | `ovnc_server_` | `ovnc_server_create()` |
| 公開API (共通) | `ovnc_` | `ovnc_pixel_format_init()` |
| 内部関数 | `ovnc__` (ダブルアンダースコア) | `ovnc__rfb_send_msg()` |
| 型 (構造体) | `ovnc_` + snake_case + `_t` | `ovnc_client_t` |
| 型 (列挙) | `OVNC_` + UPPER_SNAKE | `OVNC_ENCODING_RAW` |
| マクロ | `OVNC_` + UPPER_SNAKE | `OVNC_VERSION_MAJOR` |

### 一般規則

- 関数名: `ovnc_<module>_<action>` (snake_case)
- 構造体: `ovnc_<name>_t`
- 列挙値: `OVNC_<CATEGORY>_<VALUE>`
- コールバック型: `ovnc_<event>_fn`
- 全ての公開シンボルに `ovnc_` プレフィクスを付与（名前衝突回避）

---

## 3. モジュール構成

```
include/ovnc/
├── ovnc.h                  ← メインヘッダ（全ヘッダをインクルード）
├── ovnc_common.h           ← 共通型、エラーコード、ピクセルフォーマット
├── ovnc_client.h           ← クライアントAPI
├── ovnc_server.h           ← サーバAPI
└── ovnc_version.h          ← バージョン情報（CMakeで生成）

src/
├── common/
│   ├── ovnc_common.c       ← 共通ユーティリティ
│   ├── ovnc_pixel.c        ← ピクセルフォーマット変換
│   └── ovnc_error.c        ← エラー処理
├── protocol/
│   ├── ovnc_handshake.c    ← ハンドシェイク処理
│   ├── ovnc_messages.c     ← メッセージ送受信
│   ├── ovnc_security.c     ← セキュリティ/認証
│   └── ovnc_encoding.c     ← エンコーディング（デコード/エンコード）
│   ├── ovnc_enc_raw.c      ← Rawエンコーディング
│   ├── ovnc_enc_copyrect.c ← CopyRectエンコーディング
│   ├── ovnc_enc_rre.c      ← RREエンコーディング
│   ├── ovnc_enc_hextile.c  ← Hextileエンコーディング
│   ├── ovnc_enc_trle.c     ← TRLEエンコーディング
│   └── ovnc_enc_zrle.c     ← ZRLEエンコーディング
├── transport/
│   ├── ovnc_transport.c    ← トランスポート抽象化
│   ├── ovnc_tcp.c          ← TCP実装
│   └── ovnc_tls.c          ← TLS実装（オプション）
├── platform/
│   ├── ovnc_platform.h     ← プラットフォーム抽象化ヘッダ（内部用）
│   ├── ovnc_socket_win32.c ← Win32ソケット
│   ├── ovnc_socket_posix.c ← POSIXソケット
│   ├── ovnc_thread_win32.c ← Win32スレッド
│   └── ovnc_thread_posix.c ← POSIXスレッド (pthread)
├── client/
│   └── ovnc_client.c       ← クライアント実装
└── server/
    └── ovnc_server.c       ← サーバ実装
```

---

## 4. 共通データ型 (ovnc_common.h)

### エラーコード

```c
typedef enum {
    OVNC_OK = 0,                    /* 成功 */
    OVNC_ERR_NOMEM,                 /* メモリ不足 */
    OVNC_ERR_INVALID_ARG,           /* 無効な引数 */
    OVNC_ERR_IO,                    /* I/Oエラー */
    OVNC_ERR_TIMEOUT,               /* タイムアウト */
    OVNC_ERR_CONNECTION_REFUSED,    /* 接続拒否 */
    OVNC_ERR_CONNECTION_CLOSED,     /* 接続が閉じられた */
    OVNC_ERR_VERSION_MISMATCH,      /* バージョン不一致 */
    OVNC_ERR_AUTH_FAILED,           /* 認証失敗 */
    OVNC_ERR_AUTH_UNSUPPORTED,      /* サポートされていない認証タイプ */
    OVNC_ERR_PROTOCOL,              /* プロトコルエラー */
    OVNC_ERR_ENCODING_UNSUPPORTED,  /* サポートされていないエンコーディング */
    OVNC_ERR_ZLIB,                  /* zlib エラー */
} ovnc_error_t;
```

### RFBバージョン

```c
typedef enum {
    OVNC_RFB_VERSION_33 = 33,  /* RFB 3.3 */
    OVNC_RFB_VERSION_37 = 37,  /* RFB 3.7 */
    OVNC_RFB_VERSION_38 = 38,  /* RFB 3.8 */
} ovnc_rfb_version_t;
```

### セキュリティタイプ

```c
typedef enum {
    OVNC_SECURITY_INVALID = 0,
    OVNC_SECURITY_NONE    = 1,
    OVNC_SECURITY_VNC_AUTH = 2,
} ovnc_security_type_t;
```

### エンコーディングタイプ

```c
typedef enum {
    OVNC_ENCODING_RAW          =  0,
    OVNC_ENCODING_COPYRECT     =  1,
    OVNC_ENCODING_RRE          =  2,
    OVNC_ENCODING_HEXTILE      =  5,
    OVNC_ENCODING_TRLE         = 15,
    OVNC_ENCODING_ZRLE         = 16,
    OVNC_ENCODING_CURSOR       = -239,
    OVNC_ENCODING_DESKTOP_SIZE = -223,
} ovnc_encoding_type_t;
```

### ピクセルフォーマット

```c
typedef struct {
    uint8_t  bits_per_pixel;   /* 8, 16, or 32 */
    uint8_t  depth;            /* 有効ビット数 */
    uint8_t  big_endian;       /* 0=リトルエンディアン, 非0=ビッグエンディアン */
    uint8_t  true_color;       /* 0=カラーマップ, 非0=トゥルーカラー */
    uint16_t red_max;
    uint16_t green_max;
    uint16_t blue_max;
    uint8_t  red_shift;
    uint8_t  green_shift;
    uint8_t  blue_shift;
} ovnc_pixel_format_t;
```

### フレームバッファ

```c
typedef struct {
    uint16_t             width;
    uint16_t             height;
    ovnc_pixel_format_t  format;
    uint8_t             *data;      /* ピクセルデータ (width * height * bytesPerPixel) */
    size_t               data_size;
} ovnc_framebuffer_t;
```

### 矩形領域

```c
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ovnc_rect_t;
```

---

## 5. クライアントAPI (ovnc_client.h)

### コールバック型

```c
/* フレームバッファが確保/再確保されるとき */
typedef ovnc_error_t (*ovnc_alloc_framebuffer_fn)(
    ovnc_client_t *client,
    ovnc_framebuffer_t *fb
);

/* フレームバッファが更新されたとき (矩形単位) */
typedef void (*ovnc_framebuffer_update_fn)(
    ovnc_client_t *client,
    const ovnc_rect_t *rect
);

/* 全矩形の更新が完了したとき (1回のFramebufferUpdateメッセージ終了) */
typedef void (*ovnc_framebuffer_update_finished_fn)(
    ovnc_client_t *client
);

/* カーソル形状が変わったとき */
typedef void (*ovnc_cursor_update_fn)(
    ovnc_client_t *client,
    uint16_t hotspot_x,
    uint16_t hotspot_y,
    uint16_t width,
    uint16_t height,
    const uint8_t *pixels,
    const uint8_t *bitmask
);

/* デスクトップサイズが変わったとき */
typedef void (*ovnc_desktop_resize_fn)(
    ovnc_client_t *client,
    uint16_t width,
    uint16_t height
);

/* ベル音 */
typedef void (*ovnc_bell_fn)(ovnc_client_t *client);

/* サーバからのクリップボードテキスト */
typedef void (*ovnc_server_cut_text_fn)(
    ovnc_client_t *client,
    const char *text,
    uint32_t length
);

/* カラーマップエントリ更新 */
typedef void (*ovnc_color_map_update_fn)(
    ovnc_client_t *client,
    uint16_t first_color,
    uint16_t num_colors,
    const uint16_t *red,
    const uint16_t *green,
    const uint16_t *blue
);

/* 接続が切断されたとき */
typedef void (*ovnc_disconnected_fn)(
    ovnc_client_t *client,
    ovnc_error_t reason
);

/* パスワード要求 (VNC認証)
 *
 * ライブラリが提供するバッファにパスワードを書き込む。
 * - buf: ライブラリが確保した書き込み先バッファ
 * - buf_size: バッファサイズ (NUL終端を含む)。RFBの仕様上パスワードは最大8文字だが、
 *   バッファは余裕を持って確保される
 * - 戻り値: 成功時はパスワードの長さ (NUL終端を含まない)。失敗/キャンセル時は -1
 *
 * セキュリティ上の契約:
 * - ライブラリは認証処理完了後、バッファをゼロクリアする
 * - 呼び出し側はスタック上の一時変数等からコピーし、コピー元も速やかにゼロクリアすべき
 */
typedef int (*ovnc_get_password_fn)(
    ovnc_client_t *client,
    char *buf,
    size_t buf_size
);
```

### コールバック構造体

```c
typedef struct {
    ovnc_alloc_framebuffer_fn           alloc_framebuffer;
    ovnc_framebuffer_update_fn          framebuffer_update;
    ovnc_framebuffer_update_finished_fn framebuffer_update_finished;
    ovnc_cursor_update_fn               cursor_update;
    ovnc_desktop_resize_fn              desktop_resize;
    ovnc_bell_fn                        bell;
    ovnc_server_cut_text_fn             server_cut_text;
    ovnc_color_map_update_fn            color_map_update;
    ovnc_disconnected_fn                disconnected;
    ovnc_get_password_fn                get_password;
} ovnc_client_callbacks_t;
```

### クライアント設定

```c
/* クライアント設定
 *
 * 所有権の規約:
 * ovnc_client_create() は全フィールドをディープコピーする。
 * 呼び出し側は create() の返却後、config 構造体および
 * そのポインタが指す領域 (host, pixel_format, encodings) を
 * 自由に解放・変更してよい。
 */
typedef struct {
    const char                *host;           /* サーバホスト名/IP (deep copy) */
    uint16_t                   port;           /* ポート (デフォルト: 5900) */
    uint8_t                    shared;         /* 共有フラグ (1=共有, 0=排他) */
    ovnc_pixel_format_t       *pixel_format;   /* 要求ピクセルフォーマット (NULL=サーバデフォルト, deep copy) */
    const ovnc_encoding_type_t *encodings;     /* エンコーディング優先リスト (deep copy) */
    size_t                     num_encodings;   /* エンコーディング数 */
    uint32_t                   connect_timeout_ms; /* 接続タイムアウト (0=デフォルト) */
    void                      *user_data;      /* ユーザデータ (ポインタをそのまま保持、所有権は呼び出し側) */
} ovnc_client_config_t;
```

### 接続情報

```c
/* 接続情報
 *
 * name フィールドの契約:
 * - 常にNUL終端される
 * - サーバ名が OVNC_SERVER_NAME_MAX - 1 バイトを超える場合は切り詰められる
 * - 切り詰めが発生した場合 name_truncated が非0になる
 * - 実際の完全なサーバ名長は name_length で取得可能
 */
#define OVNC_SERVER_NAME_MAX 256

typedef struct {
    char                  name[OVNC_SERVER_NAME_MAX]; /* サーバ名 (NUL終端保証) */
    uint32_t              name_length;    /* サーバ名の実長 (切り詰め前) */
    int                   name_truncated; /* 非0: 切り詰めが発生した */
    uint16_t              width;          /* フレームバッファ幅 */
    uint16_t              height;         /* フレームバッファ高さ */
    ovnc_pixel_format_t   pixel_format;   /* サーバのピクセルフォーマット */
    ovnc_rfb_version_t    rfb_version;    /* ネゴシエーションされたバージョン */
    ovnc_security_type_t  security_type;  /* 使用されたセキュリティタイプ */
} ovnc_connection_info_t;
```

### 関数

```c
/*-------------------------------------------------------------------
 * ライフサイクル
 *-------------------------------------------------------------------*/

/* クライアントを生成 */
ovnc_client_t* ovnc_client_create(
    const ovnc_client_config_t *config,
    const ovnc_client_callbacks_t *callbacks
);

/* クライアントを破棄 (切断済みであること) */
void ovnc_client_destroy(ovnc_client_t *client);

/*-------------------------------------------------------------------
 * 接続
 *-------------------------------------------------------------------*/

/* サーバに接続 (ブロッキング: ハンドシェイク+初期化まで完了) */
ovnc_error_t ovnc_client_connect(ovnc_client_t *client);

/* 切断 */
ovnc_error_t ovnc_client_disconnect(ovnc_client_t *client);

/* 接続中かどうか */
int ovnc_client_is_connected(const ovnc_client_t *client);

/* 接続情報を取得 */
ovnc_error_t ovnc_client_get_connection_info(
    const ovnc_client_t *client,
    ovnc_connection_info_t *info
);

/*-------------------------------------------------------------------
 * メッセージ処理ループ
 *-------------------------------------------------------------------*/

/* サーバからのメッセージを1つ処理 (ブロッキング)
 * コールバックが呼ばれる。接続が閉じたらOVNC_ERR_CONNECTION_CLOSEDを返す */
ovnc_error_t ovnc_client_process_message(ovnc_client_t *client);

/* メッセージ処理ループ (接続が閉じるまでブロック) */
ovnc_error_t ovnc_client_run(ovnc_client_t *client);

/*-------------------------------------------------------------------
 * フレームバッファ更新要求
 *-------------------------------------------------------------------*/

/* フレームバッファ更新を要求 */
ovnc_error_t ovnc_client_request_update(
    ovnc_client_t *client,
    const ovnc_rect_t *rect,      /* NULL=画面全体 */
    int incremental
);

/*-------------------------------------------------------------------
 * 入力イベント
 *-------------------------------------------------------------------*/

/* キーイベントを送信 */
ovnc_error_t ovnc_client_send_key_event(
    ovnc_client_t *client,
    uint32_t key,    /* X11 keysym */
    int down         /* 1=押下, 0=解放 */
);

/* ポインタイベントを送信 */
ovnc_error_t ovnc_client_send_pointer_event(
    ovnc_client_t *client,
    uint16_t x,
    uint16_t y,
    uint8_t button_mask
);

/* クリップボードテキストを送信 */
ovnc_error_t ovnc_client_send_cut_text(
    ovnc_client_t *client,
    const char *text,
    uint32_t length
);

/*-------------------------------------------------------------------
 * ピクセルフォーマット・エンコーディング
 *-------------------------------------------------------------------*/

/* ピクセルフォーマットを変更 (接続後) */
ovnc_error_t ovnc_client_set_pixel_format(
    ovnc_client_t *client,
    const ovnc_pixel_format_t *format
);

/* エンコーディング優先リストを変更 (接続後) */
ovnc_error_t ovnc_client_set_encodings(
    ovnc_client_t *client,
    const ovnc_encoding_type_t *encodings,
    size_t num_encodings
);

/*-------------------------------------------------------------------
 * フレームバッファアクセス
 *-------------------------------------------------------------------*/

/* フレームバッファへのポインタを取得 (読み取り専用)
 *
 * スレッド安全性:
 * - コールバック内 (framebuffer_update, framebuffer_update_finished) では
 *   ロックなしで安全に読み取れる
 * - コールバック外からアクセスする場合は、利用者側でロックを取り
 *   process_message / run と並行実行しないよう排他すること
 * - alloc_framebuffer コールバック後はポインタが変わる可能性がある
 *   (古いポインタは無効になる)
 */
const ovnc_framebuffer_t* ovnc_client_get_framebuffer(
    const ovnc_client_t *client
);

/*-------------------------------------------------------------------
 * ユーティリティ
 *-------------------------------------------------------------------*/

/* ユーザデータの取得/設定 */
void* ovnc_client_get_user_data(const ovnc_client_t *client);
void  ovnc_client_set_user_data(ovnc_client_t *client, void *data);

/* エラー文字列の取得 */
const char* ovnc_error_string(ovnc_error_t error);
```

---

## 6. サーバAPI (ovnc_server.h)

### コールバック型

```c
/* クライアントが接続したとき */
typedef int (*ovnc_client_connected_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client
);  /* 0を返すと接続を拒否 */

/* クライアントが切断したとき */
typedef void (*ovnc_client_disconnected_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client
);

/* キーイベント受信 */
typedef void (*ovnc_server_key_event_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client,
    uint32_t key,
    int down
);

/* ポインタイベント受信 */
typedef void (*ovnc_server_pointer_event_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client,
    uint16_t x,
    uint16_t y,
    uint8_t button_mask
);

/* クリップボードテキスト受信 */
typedef void (*ovnc_server_cut_text_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client,
    const char *text,
    uint32_t length
);

/* パスワード検証 (VNC認証) */
typedef int (*ovnc_verify_password_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client,
    const char *password
);  /* 0=失敗, 非0=成功 */
```

### サーバ設定

```c
typedef struct {
    const char              *name;           /* デスクトップ名 */
    uint16_t                 port;           /* リッスンポート (デフォルト: 5900) */
    uint16_t                 width;          /* フレームバッファ幅 */
    uint16_t                 height;         /* フレームバッファ高さ */
    ovnc_pixel_format_t      pixel_format;   /* ピクセルフォーマット */
    ovnc_security_type_t    *security_types; /* サポートするセキュリティタイプ */
    size_t                   num_security_types;
    void                    *user_data;
} ovnc_server_config_t;
```

### サーバコールバック構造体

```c
typedef struct {
    ovnc_client_connected_fn     client_connected;
    ovnc_client_disconnected_fn  client_disconnected;
    ovnc_server_key_event_fn     key_event;
    ovnc_server_pointer_event_fn pointer_event;
    ovnc_server_cut_text_fn      cut_text;
    ovnc_verify_password_fn      verify_password;
} ovnc_server_callbacks_t;
```

### 関数

```c
/*-------------------------------------------------------------------
 * ライフサイクル
 *-------------------------------------------------------------------*/

ovnc_server_t* ovnc_server_create(
    const ovnc_server_config_t *config,
    const ovnc_server_callbacks_t *callbacks
);

void ovnc_server_destroy(ovnc_server_t *server);

/*-------------------------------------------------------------------
 * サーバ制御
 *-------------------------------------------------------------------*/

/* リッスン開始 */
ovnc_error_t ovnc_server_start(ovnc_server_t *server);

/* サーバ停止 */
ovnc_error_t ovnc_server_stop(ovnc_server_t *server);

/* メッセージ処理ループ */
ovnc_error_t ovnc_server_run(ovnc_server_t *server);

/*-------------------------------------------------------------------
 * フレームバッファ操作
 *-------------------------------------------------------------------*/

/* フレームバッファへのポインタ取得 (書き込み可能) */
ovnc_framebuffer_t* ovnc_server_get_framebuffer(ovnc_server_t *server);

/* 指定領域を全クライアントに更新通知
 * (次のFramebufferUpdateRequest応答時に送信される) */
ovnc_error_t ovnc_server_mark_rect_modified(
    ovnc_server_t *server,
    const ovnc_rect_t *rect
);

/* フレームバッファサイズ変更 */
ovnc_error_t ovnc_server_resize(
    ovnc_server_t *server,
    uint16_t width,
    uint16_t height
);

/*-------------------------------------------------------------------
 * クライアントメッセージ送信
 *-------------------------------------------------------------------*/

/* ベル音を全クライアントに送信 */
ovnc_error_t ovnc_server_send_bell(ovnc_server_t *server);

/* クリップボードテキストを全クライアントに送信 */
ovnc_error_t ovnc_server_send_cut_text(
    ovnc_server_t *server,
    const char *text,
    uint32_t length
);

/*-------------------------------------------------------------------
 * ユーティリティ
 *-------------------------------------------------------------------*/

void* ovnc_server_get_user_data(const ovnc_server_t *server);
void  ovnc_server_set_user_data(ovnc_server_t *server, void *data);
```

---

## 7. エラーハンドリングパターン

### 基本パターン

すべての関数が `ovnc_error_t` を返す。成功は `OVNC_OK` (0)。

```c
ovnc_error_t err = ovnc_client_connect(client);
if (err != OVNC_OK) {
    fprintf(stderr, "接続失敗: %s\n", ovnc_error_string(err));
    ovnc_client_destroy(client);
    return 1;
}
```

### コールバックでのエラー

コールバック関数はエラーを返さない（void）。ただし `alloc_framebuffer` は
メモリ確保の成否を返す。

---

## 8. 使用例: クライアント

```c
#include <ovnc/ovnc.h>
#include <stdio.h>

static void on_framebuffer_update(ovnc_client_t *client, const ovnc_rect_t *rect)
{
    printf("更新: (%d,%d) %dx%d\n", rect->x, rect->y, rect->width, rect->height);
}

static void on_update_finished(ovnc_client_t *client)
{
    /* 次の更新を要求 */
    ovnc_client_request_update(client, NULL, 1);
}

static int on_get_password(ovnc_client_t *client, char *buf, size_t buf_size)
{
    const char *pw = "mypassword";
    size_t len = strlen(pw);
    if (len >= buf_size) len = buf_size - 1;
    memcpy(buf, pw, len);
    buf[len] = '\0';
    return (int)len;
}

int main(void)
{
    ovnc_encoding_type_t encodings[] = {
        OVNC_ENCODING_ZRLE,
        OVNC_ENCODING_TRLE,
        OVNC_ENCODING_COPYRECT,
        OVNC_ENCODING_RAW,
        OVNC_ENCODING_CURSOR,
        OVNC_ENCODING_DESKTOP_SIZE,
    };

    ovnc_client_config_t config = {
        .host = "192.168.1.100",
        .port = 5900,
        .shared = 1,
        .pixel_format = NULL,  /* サーバデフォルトを使用 */
        .encodings = encodings,
        .num_encodings = sizeof(encodings) / sizeof(encodings[0]),
    };

    ovnc_client_callbacks_t callbacks = {
        .framebuffer_update = on_framebuffer_update,
        .framebuffer_update_finished = on_update_finished,
        .get_password = on_get_password,
    };

    ovnc_client_t *client = ovnc_client_create(&config, &callbacks);
    if (!client) {
        fprintf(stderr, "クライアント生成失敗\n");
        return 1;
    }

    ovnc_error_t err = ovnc_client_connect(client);
    if (err != OVNC_OK) {
        fprintf(stderr, "接続失敗: %s\n", ovnc_error_string(err));
        ovnc_client_destroy(client);
        return 1;
    }

    /* 最初の全画面更新を要求 */
    ovnc_client_request_update(client, NULL, 0);

    /* メッセージ処理ループ (切断まで) */
    ovnc_client_run(client);

    ovnc_client_destroy(client);
    return 0;
}
```

---

## 9. 使用例: 録画アプリケーション (ライブラリ外)

```c
/* 録画はライブラリ外のアプリケーション層で行う */
#include <ovnc/ovnc.h>
/* + FFmpegなどの録画ライブラリ */

typedef struct {
    /* FFmpegコンテキスト等 */
    void *recorder;
} app_context_t;

static void on_update_finished(ovnc_client_t *client)
{
    app_context_t *ctx = ovnc_client_get_user_data(client);
    const ovnc_framebuffer_t *fb = ovnc_client_get_framebuffer(client);

    /* フレームバッファからフレームを録画 */
    /* recorder_write_frame(ctx->recorder, fb->data, fb->width, fb->height); */

    /* 次の更新を即座に要求 */
    ovnc_client_request_update(client, NULL, 1);
}
```

---

## 10. ビルドシステム設計

### ディレクトリ構成 (最終形)

```
libopenvnc/
├── CMakeLists.txt
├── cmake/
│   └── ovnc_version.h.in      ← バージョンテンプレート
├── include/
│   └── ovnc/
│       ├── ovnc.h
│       ├── ovnc_common.h
│       ├── ovnc_client.h
│       ├── ovnc_server.h
│       └── ovnc_version.h      ← CMakeで生成
├── src/
│   ├── common/
│   ├── protocol/
│   ├── transport/
│   ├── platform/
│   ├── client/
│   └── server/
├── tests/
│   ├── CMakeLists.txt
│   ├── test_handshake.c
│   ├── test_encoding.c
│   └── ...
├── examples/
│   ├── simple_viewer.c
│   └── ...
├── docs/
│   ├── rfb-protocol.md
│   ├── rfb-version-differences.md
│   └── api-design.md
├── LICENSE
├── README.md
└── CLAUDE.md
```

### CMakeターゲット

```cmake
# 共有ライブラリ / 静的ライブラリ
add_library(ovnc ...)
add_library(ovnc_static STATIC ...)

# オプション
option(OVNC_BUILD_TESTS "テストをビルド" ON)
option(OVNC_BUILD_EXAMPLES "サンプルをビルド" ON)
option(OVNC_ENABLE_TLS "TLSサポートを有効化" OFF)
option(OVNC_ENABLE_VNC_AUTH "VNC認証 (DES) を有効化" ON)
```

### 外部依存

| ライブラリ | 用途 | 必須/オプション |
|-----------|------|----------------|
| zlib | ZRLE圧縮展開 | 必須 |
| OpenSSL / mbedTLS | TLS (暗号化トランスポート) | オプション (`OVNC_ENABLE_TLS=ON` 時) |

### ビルドオプションと機能マトリクス

`OVNC_ENABLE_TLS` と `OVNC_ENABLE_VNC_AUTH` は独立したオプションとして扱う。

| OVNC_ENABLE_VNC_AUTH | OVNC_ENABLE_TLS | DES実装 | セキュリティタイプ |
|---------------------|-----------------|---------|-------------------|
| OFF | OFF | なし | None のみ |
| ON | OFF | 内蔵DES | None, VNC Authentication |
| OFF | ON | なし | None + TLS拡張 |
| ON | ON | OpenSSL/mbedTLSのDES | None, VNC Authentication + TLS拡張 |

**VNC認証のDES実装について:**

- `OVNC_ENABLE_VNC_AUTH=ON` かつ `OVNC_ENABLE_TLS=OFF` の場合:
  最小限のDES実装を内蔵する（RFBのVNC認証専用、汎用暗号ライブラリとしては提供しない）
- `OVNC_ENABLE_VNC_AUTH=ON` かつ `OVNC_ENABLE_TLS=ON` の場合:
  外部暗号ライブラリ (OpenSSL/mbedTLS) のDESを使用
- RFC 6143自体がVNC認証を「暗号学的に脆弱」と明記しているため、
  内蔵DES実装でも実用上の問題は少ない
- CI構成では全4パターンをビルド・テストする
