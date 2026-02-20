# RFBバージョン差異 (3.3 / 3.7 / 3.8)

本ドキュメントはRFBプロトコルの各バージョンにおける動作差異を実装視点でまとめたものである。

---

## 1. バージョン概要

| バージョン | 文字列 | RFC | 特徴 |
|------------|--------|-----|------|
| 3.3 | `RFB 003.003\n` | なし | 初期版。最小限のハンドシェイク |
| 3.7 | `RFB 003.007\n` | なし | セキュリティタイプのネゴシエーション追加 |
| 3.8 | `RFB 003.008\n` | RFC 6143 | エラーハンドリング改善。標準化版 |

不明な3.xバージョンはすべて3.3として扱う。

---

## 2. セキュリティハンドシェイクの差異

### RFB 3.3

```
サーバ → クライアント:
┌─────────┬──────┬────────────────────────────┐
│ 4       │ U32  │ security-type              │
└─────────┴──────┴────────────────────────────┘
```

- サーバが一方的にセキュリティタイプを決定（ネゴシエーションなし）
- 値: 0 (失敗), 1 (None), 2 (VNC Authentication)
- 値0の場合: reason-length (U32) + reason-string が続き、接続閉じ

### RFB 3.7 / 3.8

```
サーバ → クライアント:
┌─────────┬──────┬────────────────────────────┐
│ 1       │ U8   │ number-of-security-types   │
│ 可変    │ U8[] │ security-types (リスト)     │
└─────────┴──────┴────────────────────────────┘

クライアント → サーバ:
┌─────────┬──────┬────────────────────────────┐
│ 1       │ U8   │ security-type (選択)        │
└─────────┴──────┴────────────────────────────┘
```

- サーバがサポートするセキュリティタイプのリストを送信
- クライアントがその中から1つを選択
- number-of-security-types = 0 の場合: 接続失敗（reason-stringが続く）

---

## 3. SecurityResultの差異

### SecurityResultメッセージ

```
┌─────────┬──────┬────────────────────────────┐
│ 4       │ U32  │ status (0=OK, 1=failed)    │
└─────────┴──────┴────────────────────────────┘

失敗時 (3.8のみ):
┌─────────┬──────┬────────────────────────────┐
│ 4       │ U32  │ reason-length              │
│ 可変    │ U8[] │ reason-string              │
└─────────┴──────┴────────────────────────────┘
```

### バージョン別動作

| セキュリティタイプ | 3.3 | 3.7 | 3.8 |
|-------------------|-----|-----|-----|
| None (1) — 成功時 | SecurityResultなし → ClientInit | SecurityResultなし → ClientInit | SecurityResult (status=0) → ClientInit |
| VNC Auth (2) — 成功時 | SecurityResult (status=0) → ClientInit | SecurityResult (status=0) → ClientInit | SecurityResult (status=0) → ClientInit |
| VNC Auth (2) — 失敗時 | SecurityResult (status=1) → 接続閉じ | SecurityResult (status=1) → 接続閉じ | SecurityResult (status=1) + reason → 接続閉じ |

**まとめ:**
- 3.3/3.7: None認証ではSecurityResultをスキップ
- 3.3/3.7: VNC認証失敗時にエラー理由文字列なし
- 3.8: 全ケースでSecurityResultを送信し、失敗時は必ずエラー理由を含む

---

## 4. 実装上の分岐ポイント

### ハンドシェイク処理のフローチャート

```
バージョンネゴシエーション完了
        │
        ▼
  ┌─────────────┐
  │ version == ? │
  └─────────────┘
        │
   ┌────┼────┐
   │    │    │
  3.3  3.7  3.8
   │    │    │
   ▼    ▼    ▼
  U32  リスト リスト   ← セキュリティタイプの受信方法
  受信  受信   受信
   │    │    │
   │    ▼    ▼
   │   タイプ タイプ   ← セキュリティタイプの選択
   │   選択   選択
   │    │    │
   ▼    ▼    ▼
  認証  認証  認証     ← 認証交換（タイプに依存）
   │    │    │
   │    │    │
   ▼    ▼    ▼
  ┌─────────────────┐
  │ type == None ?   │
  └─────────────────┘
   │           │
  Yes         No (VNC Auth)
   │           │
   ▼           ▼
  ┌───────────────────────────┐
  │ version == 3.8 ?          │
  └───────────────────────────┘
   │              │
  Yes (3.8)      No (3.3/3.7)
   │              │
   ▼              │
  SecurityResult  │
  受信            │
   │              │
   │         ┌────┘
   │         │
   │    SecurityResult   ← VNC Authの場合のみ
   │    受信
   │         │
   ▼         ▼
  ┌───────────────────────────┐
  │ version == 3.8 &&         │
  │ status == failed ?        │
  └───────────────────────────┘
   │              │
  Yes            No
   │              │
   ▼              ▼
  reason-string  ClientInit
  受信 → 終了    送信
```

### コードでの判定ロジック (疑似コード)

```c
// セキュリティハンドシェイク
if (version == RFB_33) {
    // サーバがU32でタイプを通知
    security_type = read_u32();
    if (security_type == 0) {
        // 接続失敗
        reason = read_reason_string();
        disconnect();
    }
} else {
    // 3.7, 3.8: ネゴシエーション方式
    count = read_u8();
    if (count == 0) {
        reason = read_reason_string();
        disconnect();
    }
    types = read_bytes(count);
    selected_type = choose_security_type(types);
    write_u8(selected_type);
}

// 認証交換（タイプに依存）
perform_authentication(security_type);

// SecurityResult
if (version == RFB_38) {
    // 3.8: 全タイプでSecurityResult
    result = read_u32();
    if (result != 0) {
        reason = read_reason_string();
        disconnect();
    }
} else if (security_type == VNC_AUTH) {
    // 3.3/3.7: VNC Authの場合のみSecurityResult
    result = read_u32();
    if (result != 0) {
        // エラー理由なし
        disconnect();
    }
}
// else: 3.3/3.7のNoneではSecurityResultなし

// 初期化フェーズへ
send_client_init();
recv_server_init();
```

---

## 5. 互換性に関するメモ

- クライアントはサーバの最高バージョン以下を選択する
- サーバが3.8でクライアントが3.3を返すことも有効
- 不明な3.xバージョン（例: 3.5, 3.14）は3.3として扱う
- 3.7は実質的に3.8の「SecurityResultが不完全な版」と考えてよい
- 実装では3.3と3.7/3.8を大きな分岐点として扱い、3.7と3.8の差はSecurityResultの有無で分岐する
