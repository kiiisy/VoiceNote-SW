/**
 * @file shared_mm.h
 * @brief CPU0/CPU1間のIPC用の共有メッセージ定義（固定長128Byte）
 */
#pragma once

// 標準ライブラリ
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
    Stop     = 11,  // 停止（未使用）
    RecStart = 12,  // 録音開始
    RecStop  = 13,  // 録音停止
    Pause    = 14,  // 一時停止
    Resume   = 15,  // 再開

    ListDir = 20,  // SD取得

    SetRecOption = 30,  // 録音オプション設定
    SetNoiseGate = 31,  // ノイズゲート設定
    SetAgc       = 32,  // 測距連動設定
    SetEffect    = 33,  // エフェクト設定
    SetAutoRec   = 34,  // 自動録音設定
};

/**
 * @brief CPU0/CPU1が自発的に送るイベントID（通知/結果/ACK など）
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

/**
 * @brief IPC メッセージ共通ヘッダ
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
 * @brief 固定長IPCメッセージ本体（128Byte）
 */
struct alignas(4) Msg
{
    MsgHdr  header{};                  // メッセージヘッダ
    uint8_t payload[kPayloadBytes]{};  // ペイロード
};
static_assert(sizeof(Msg) == kMsgBytes, "Msg must be 128 bytes");

/**
 * @brief Ackの状態
 */
enum class AckStatus : uint16_t
{
    Accepted = 1,  // 受信完了
    Done     = 2   // 完了
};

/**
 * @brief Ackイベントのpayload
 */
struct AckPayload
{
    uint16_t req_cmd_id;   // CmdId
    uint16_t status;       // AckStatus
    uint32_t seq;          // same as MsgHdr::seq
    int32_t  result_code;  // 0=OK, <0 error
};
static_assert(sizeof(AckPayload) <= kPayloadBytes);

/**
 * @brief Errorイベントのpayload
 */
struct ErrorPayload
{
    uint16_t req_cmd_id;  // CmdId
    uint16_t error_code;  // project-defined
    uint32_t seq;
    int32_t  detail;  // optional
};
static_assert(sizeof(ErrorPayload) <= kPayloadBytes);

/**
 * @brief ResultChunkの種類
 */
enum class ResultKind : uint16_t
{
    DirEntry = 1  // ディレクトリエントリ
};

/**
 * @brief 分割結果（ResultChunk）の共通ヘッダ
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
 * @brief ListDirのDirEntry
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
 * @brief SetRecOptionコマンド
 */
struct RecOptionPayload
{
    uint8_t dc_enable;      // DCカット有効無効
    uint8_t ng_enable;      // ノイズゲート有効無効
    uint8_t reserved0[2];   // 予約
    int32_t dc_fc_q16;      // DCカットオフ周波数(Hz, Q16)
    int32_t ng_th_open_q15; // ノイズゲート開閾値(Q15)
    int32_t ng_th_close_q15;// ノイズゲート閉閾値(Q15)
    uint16_t ng_attack_ms;  // アタック時間[ms]
    uint16_t ng_release_ms; // リリース時間[ms]
};
static_assert(sizeof(RecOptionPayload) <= kPayloadBytes);

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

/**
 * @brief 任意のコマンドをraw payloadとして詰めてMsgを生成する
 *
 * @param id    コマンドID
 * @param seq   シーケンス番号
 * @param payload payload先頭（nullptr可）
 * @param len   payloadバイト数
 * @param flags 将来拡張フラグ
 * @return 生成されたMsg（128Byte固定）
 */
inline Msg MakeCmdRaw(CmdId id, uint32_t seq, const void *payload, uint16_t len, uint32_t flags = 0)
{
    Msg msg{};

    uint16_t payload_len = len;
    if (payload_len > kPayloadBytes) {
        payload_len = static_cast<uint16_t>(kPayloadBytes);
    }
    if (!payload) {
        payload_len = 0;
    }

    msg.header.id    = static_cast<uint16_t>(id);
    msg.header.len   = payload_len;
    msg.header.seq   = seq;
    msg.header.flags = flags;

    if (payload_len != 0) {
        std::memcpy(msg.payload, payload, payload_len);
    }

    return msg;
}

/**
 * @brief 任意のイベントをraw payloadとして詰めてMsgを生成する
 *
 * @param id    イベントID
 * @param seq   シーケンス番号
 * @param payload payload先頭（nullptr可）
 * @param len   payloadバイト数
 * @param flags 将来拡張フラグ
 * @return 生成されたMsg（128Byte固定）
 */
inline Msg MakeEvtRaw(EvtId id, uint32_t seq, const void *payload, uint16_t len, uint32_t flags = 0)
{
    Msg      msg{};
    uint16_t payload_len = len;
    if (payload_len > kPayloadBytes) {
        payload_len = static_cast<uint16_t>(kPayloadBytes);
    }
    if (!payload) {
        payload_len = 0;
    }

    msg.header.id    = static_cast<uint16_t>(id);
    msg.header.len   = payload_len;
    msg.header.seq   = seq;
    msg.header.flags = flags;

    if (payload_len != 0) {
        std::memcpy(msg.payload, payload, payload_len);
    }

    return msg;
}

/**
 * @brief 型付きpayloadをコマンドとしてMsgに詰める
 *
 * @tparam Payload payloadの型
 * @param id    コマンドID
 * @param seq   シーケンス番号
 * @param payload  payload
 * @param flags 将来拡張フラグ
 */
template <typename Payload> inline Msg MakeCmd(CmdId id, uint32_t seq, const Payload &payload, uint32_t flags = 0)
{
    static_assert(sizeof(Payload) <= kPayloadBytes, "payload too big");
    return MakeCmdRaw(id, seq, &payload, (uint16_t)sizeof(Payload), flags);
}

/**
 * @brief 型付きpayloadをイベントとしてMsgに詰める
 *
 * @tparam Payload payloadの型
 * @param id    イベントID
 * @param seq   シーケンス番号
 * @param payload  payload
 * @param flags 将来拡張フラグ
 */
template <typename Payload> inline Msg MakeEvt(EvtId id, uint32_t seq, const Payload &payload, uint32_t flags = 0)
{
    static_assert(sizeof(Payload) <= kPayloadBytes, "payload too big");
    return MakeEvtRaw(id, seq, &payload, (uint16_t)sizeof(Payload), flags);
}

/**
 * @brief Msgのpayloadを型として安全に参照する
 *
 * @tparam 期待するpayload型
 * @param msg メッセージ
 * @param expected_id 期待するheader.id値（uint16_t）
 * @return 条件一致ならpayload先頭へのポインタ、違えばnullptr
 */
template <typename Payload> inline const Payload *AsPayload(const Msg &msg, uint16_t expected_id)
{
    if (msg.header.id != expected_id) {
        return nullptr;
    }

    if (msg.header.len != sizeof(Payload)) {
        return nullptr;
    }

    return reinterpret_cast<const Payload *>(msg.payload);
}

}  // namespace ipc
}  // namespace core
