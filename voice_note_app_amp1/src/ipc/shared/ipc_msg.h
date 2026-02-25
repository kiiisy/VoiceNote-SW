/**
 * @file shared_mm.h
 * @brief CPU0/CPU1 間 IPC 用の共有メッセージ定義（固定長 128B）
 *
 * このヘッダは、OCM High / DDR などの共有メモリ上に配置する「固定長メッセージ」の
 * レイアウトを統一するためのもの。
 *
 * 設計方針:
 * - 1メッセージ = 128B 固定（リングバッファ等で扱いやすい）
 * - ヘッダ + payload の単純構造
 * - payload は「型付き struct」を memcpy で詰める（アラインメント事故を避ける）
 * - Unpack 時は id / len を厳格チェックしてから reinterpret_cast する
 *
 * 注意:
 * - この定義は CPU0/CPU1 両方で完全一致が必須（ABI/packing/alignas に注意）
 * - payload 内の文字列は '\0' 終端を推奨（固定長で運用）
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace core {
namespace ipc {

/**
 * @brief CPU0->CPU1（またはその逆）へ送るコマンド ID
 */
enum class CmdId : uint16_t
{
    Play     = 10,  // 再生
    Stop     = 11,  // 停止
    RecStart = 12,  // 録音開始
    RecStop  = 13,  // 録音停止
    Pause    = 14,  // 一時停止
    Resume   = 15,  // 再開

    ListDir = 20,  // SD取得

    SetDcCut     = 30,  // DCカット設定
    SetNoiseGate = 31,  // ノイズゲート設定
    SetAgc       = 32,  // 測距連動設定
    SetEffect    = 33,  // エフェクト設定
    SetAutoRec   = 34,  // 自動録音設定
};

/**
 * @brief CPU0/CPU1 が自発的に送るイベント ID（通知/結果/ACK など）
 */
enum class EvtId : uint16_t
{
    Ack         = 10,
    Error       = 11,
    ResultChunk = 12,

    PlaybackStatus = 20,
    RecordStatus   = 21,
};

/**
 * @brief PlaybackStatusPayloadの状態ID
 */
enum class PlaybackStsId : uint8_t
{
    Stop  = 0,
    Play  = 1,
    Pause = 2,
};

/**
 * @brief RecordStatusPayloadの状態ID
 */
enum class RecStsId : uint8_t
{
    Stop = 0,
    Rec  = 1,
};

// --------------------
// Msg (128B固定)
// --------------------
/**
 * @brief IPC メッセージ共通ヘッダ
 *
 * id は CmdId / EvtId のいずれかを uint16_t で保持する。
 * len は payload の有効バイト数。
 */
struct alignas(4) MsgHdr
{
    uint16_t id;     // CmdId or EvtId
    uint16_t len;    // payload bytes
    uint32_t seq;    // request sequence
    uint32_t flags;  // 使用予定未定
};

static constexpr size_t kMsgBytes     = 128;                         // 1メッセージの固定サイズ（バイト）
static constexpr size_t kPayloadBytes = kMsgBytes - sizeof(MsgHdr);  // payload の最大サイズ（バイト）

/**
 * @brief 固定長 IPC メッセージ本体（128Byte）
 */
struct alignas(4) Msg
{
    MsgHdr  header{};                  // メッセージヘッダ
    uint8_t payload[kPayloadBytes]{};  // ペイロード
};
static_assert(sizeof(Msg) == kMsgBytes, "Msg must be 128 bytes");

// --------------------
// Result/Ack/Error 共通
// --------------------
/**
 * @brief ACK の状態
 */
enum class AckStatus : uint16_t
{
    Accepted = 1,  // 受信完了
    Done     = 2   // 完了
};

/**
 * @brief Ack イベントの payload
 *
 * 同一 seq を持つコマンドに対する進捗/完了通知として使用する。
 */
struct AckPayload
{
    uint16_t req_cmd_id;   // (uint16_t)CmdId
    uint16_t status;       // (uint16_t)AckStatus
    uint32_t seq;          // same as MsgHdr::seq (冗長だがデバッグしやすい)
    int32_t  result_code;  // 0=OK, <0 error (Errorと分けるなら0固定でもOK)
};
static_assert(sizeof(AckPayload) <= kPayloadBytes);

/**
 * @brief Error イベントの payload
 */
struct ErrorPayload
{
    uint16_t req_cmd_id;  // (uint16_t)CmdId
    uint16_t error_code;  // project-defined
    uint32_t seq;
    int32_t  detail;  // optional
};
static_assert(sizeof(ErrorPayload) <= kPayloadBytes);

// --------------------
// SD ListDir: ResultChunk
// --------------------
/**
 * @brief ResultChunk の種類
 */
enum class ResultKind : uint16_t
{
    DirEntry = 1  // ディレクトリエントリ
};

/**
 * @brief 分割結果（ResultChunk）の共通ヘッダ
 *
 * index は 0..N-1 の連番で送る想定。
 * total_hint は総数が分かるなら入れる（不明なら 0）。
 */
struct ResultChunkHdr
{
    uint16_t req_cmd_id;  // (uint16_t)CmdId::ListDir
    uint16_t kind;        // (uint16_t)ResultKind
    uint32_t seq;         // 対象コマンドの seq
    uint16_t index;       // 0..N-1
    uint16_t total_hint;  // 0 if unknown
};

/**
 * @brief ListDirのDirEntry（128B payload に収まる固定長）
 *
 * name は固定長で運用し、終端 '\0' を推奨。
 */
struct DirEntryPayload
{
    ResultChunkHdr h;            // 分割結果ヘッダ
    uint32_t       size;         // ファイルサイズ
    uint8_t        attr;         // 0=file, 1=dir など
    uint8_t        reserved[3];  // 予約
    char           name[80];     //ファイル/ディレクトリ名
};
static_assert(sizeof(DirEntryPayload) <= kPayloadBytes);

// --------------------
// Playback status
// --------------------
/**
 * @brief 再生状態のpayload
 */
struct PlaybackStatusPayload
{
    uint32_t position_ms;  // 現在位置
    uint32_t duration_ms;  // 総時間
    uint32_t remain_ms;    // 残り
    uint8_t  state;        // 0=stop, 1=play, 2=pause
    uint8_t  reserved[3];  // 予約
};
static_assert(sizeof(PlaybackStatusPayload) <= kPayloadBytes);

// --------------------
// Record status
// --------------------
/**
 * @brief 録音状態のpayload
 */
struct RecordStatusPayload
{
    uint32_t captured_ms;  // 録音経過
    uint32_t target_ms;    // 目標（固定30秒なら30000）
    uint32_t remain_ms;    // 残り
    uint8_t  state;        // 0=stop,1=rec
    uint8_t  reserved[3];  // 予約
};
static_assert(sizeof(RecordStatusPayload) <= kPayloadBytes);

// --------------------
// Command payloads
// --------------------
/**
 * @brief Playコマンド
 */
struct PlayPayload
{
    uint32_t track_id;      // トラックID
    char     filename[64];  // ファイル名
};
static_assert(sizeof(PlayPayload) <= kPayloadBytes);

/**
 * @brief RecStartコマンド
 */
struct RecStartPayload
{
    char     filename[64];    // ファイル名
    uint32_t sample_rate_hz;  // サンプリング
    uint16_t bits;            // 0..1000 etc
    uint16_t ch;              // 予約
};
static_assert(sizeof(RecStartPayload) <= kPayloadBytes);

/**
 * @brief ListDirコマンド
 */
struct ListDirPayload
{
    char path[80];  // パス
};
static_assert(sizeof(ListDirPayload) <= kPayloadBytes);

/**
 * @brief SetDcCutコマンド
 */
struct SetDcCutPayload
{
    uint8_t enable;       // 有効無効
    uint8_t reserved[3];  // 予約
    int32_t fc_q16;       // カットオフ周波数
};
static_assert(sizeof(SetDcCutPayload) <= kPayloadBytes);

/**
 * @brief SetAgc（距離連動）コマンド
 */
struct SetAgcPayload
{
    uint8_t enable;        // 有効無効
    uint8_t reserved0[3];  // 予約

    int16_t dist_mm;        // 200..3000
    int16_t min_gain_x100;  // 50..200 (0.50x..2.00x)
    int16_t max_gain_x100;  // 50..200
    int16_t speed_k;        // alpha=1/2^k

    uint16_t reserved1;  // 予約
};
static_assert(sizeof(SetAgcPayload) <= kPayloadBytes);

// --------------------
// Pack helpers（型安全）
// --------------------
/**
 * @brief 任意のコマンドを raw payload として詰めて Msg を生成する
 *
 * @param id    コマンドID
 * @param seq   シーケンス番号（要求-応答の対応付け）
 * @param payload payload 先頭（nullptr 可）
 * @param len   payload バイト数
 * @param flags 将来拡張フラグ
 * @return 生成された Msg（128B 固定）
 */
inline Msg MakeCmdRaw(CmdId id, uint32_t seq, const void *payload, uint16_t len, uint32_t flags = 0)
{
    Msg m{};
    m.header.id    = static_cast<uint16_t>(id);
    m.header.len   = len;
    m.header.seq   = seq;
    m.header.flags = flags;

    if (payload && len) {
        if (len > kPayloadBytes) {
            len = (uint16_t)kPayloadBytes;  // 念のため
        }
        std::memcpy(m.payload, payload, len);
    }

    return m;
}

/**
 * @brief 任意のイベントを raw payload として詰めて Msg を生成する
 *
 * @param id    イベントID
 * @param seq   シーケンス番号（運用で 0 可。対応付けするなら埋める）
 * @param payload payload 先頭（nullptr 可）
 * @param len   payload バイト数
 * @param flags 将来拡張フラグ
 * @return 生成された Msg（128B 固定）
 */
inline Msg MakeEvtRaw(EvtId id, uint32_t seq, const void *payload, uint16_t len, uint32_t flags = 0)
{
    Msg m{};
    m.header.id    = static_cast<uint16_t>(id);
    m.header.len   = len;
    m.header.seq   = seq;
    m.header.flags = flags;

    if (payload && len) {
        if (len > kPayloadBytes) {
            len = (uint16_t)kPayloadBytes;
        }
        std::memcpy(m.payload, payload, len);
    }

    return m;
}

/**
 * @brief 型付き payload をコマンドとして Msg に詰める
 *
 * @tparam Payload payload の型
 * @param id    コマンドID
 * @param seq   シーケンス番号
 * @param p     payload 本体
 * @param flags 将来拡張フラグ
 */
template <typename Payload> inline Msg MakeCmd(CmdId id, uint32_t seq, const Payload &p, uint32_t flags = 0)
{
    static_assert(sizeof(Payload) <= kPayloadBytes, "payload too big");
    return MakeCmdRaw(id, seq, &p, (uint16_t)sizeof(Payload), flags);
}

/**
 * @brief 型付き payload をイベントとして Msg に詰める
 *
 * @tparam Payload payload の型
 * @param id    イベントID
 * @param seq   シーケンス番号
 * @param p     payload 本体
 * @param flags 将来拡張フラグ
 */
template <typename Payload> inline Msg MakeEvt(EvtId id, uint32_t seq, const Payload &p, uint32_t flags = 0)
{
    static_assert(sizeof(Payload) <= kPayloadBytes, "payload too big");
    return MakeEvtRaw(id, seq, &p, (uint16_t)sizeof(Payload), flags);
}

/**
 * @brief Msg の payload を型として安全に参照する（id/len チェック付き）
 *
 * expected_id は CmdId/EvtId を uint16_t にした値を渡す想定。
 *
 * @tparam Payload 期待する payload 型
 * @param m メッセージ
 * @param expected_id 期待する header.id 値（uint16_t）
 * @return 条件一致なら payload 先頭へのポインタ、違えば nullptr
 */
template <typename Payload> inline const Payload *AsPayload(const Msg &m, uint16_t expected_id)
{
    if (m.header.id != expected_id) {
        return nullptr;
    }
    if (m.header.len != sizeof(Payload)) {
        return nullptr;
    }

    return reinterpret_cast<const Payload *>(m.payload);
}
}  // namespace ipc
}  // namespace core
