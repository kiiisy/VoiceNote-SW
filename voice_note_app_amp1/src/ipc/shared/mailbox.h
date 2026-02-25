/**
 * @file mailbox.h
 * @brief CPU0/CPU1 共有メモリ上の IPC（リングバッファ + 送受ラッパ）
 *
 * 共有メモリ上に SharedIpc を配置し、cmd/evt の 2 本の SPSC リングで
 * 双方向通信を行う。
 *
 * 構成:
 * - SharedIpc::cmd : CPU0 -> CPU1（CmdTx/CmdRx）
 * - SharedIpc::evt : CPU1 -> CPU0（EvtTx/EvtRx）
 *
 * キャッシュ/属性:
 * - 共有領域は non-cache（または適切に flush/invalidate）であることが前提。
 * - alignas(64) は false sharing を避けるための意図。
 */

#pragma once

// 標準ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "ipc_msg.h"
#include "shared_mm.h"

namespace core {
namespace ipc {

// Ring parameters
static constexpr uint32_t kRingN = 256;  // リングスロット数
static_assert((kRingN & (kRingN - 1u)) == 0u, "kRingN must be power-of-two");

/**
 * @brief SPSC リング本体（共有メモリ上に配置）
 *
 * head/tail は別コアから更新されるため volatile。
 * _pad は head/tail と slots のキャッシュライン分離の意図。
 */
struct alignas(64) Ring
{
    volatile uint32_t head;           // producer
    volatile uint32_t tail;           // consumer
    uint32_t          _pad[14];       // パディング
    Msg               slots[kRingN];  // 固定長 Msg のスロット配列
};

/**
 * @brief 共有 IPC 領域のトップ構造体
 *
 * magic/version は初期化確認や互換性チェックに使用する想定。
 * cmd/evt は片方向 SPSC リング。
 */
struct alignas(64) SharedIpc
{
    uint32_t magic;      // 共有領域の識別子
    uint32_t version;    // 構造体バージョン
    uint32_t _rsv0[14];  // 64B ヘッダ領域
    Ring     cmd;        // CPU0 -> CPU1
    Ring     evt;        // CPU1 -> CPU0
};

/**
 * @brief 共有 IPC 領域へのポインタを取得する
 *
 * @return 共有メモリ上の SharedIpc ポインタ
 */
inline SharedIpc *GetShared() { return reinterpret_cast<SharedIpc *>(kSharedBase); }

/**
 * @brief SPSC リング操作クラス（共有 Ring に対する Push/Pop）
 */
class RingSpsc
{
public:
    explicit RingSpsc(Ring *r) : r_(r) {}
    bool Push(const Msg &m);
    bool Pop(Msg &m);

private:
    Ring *r_;  // 操作対象リング
};

/**
 * @brief cmd リング送信（CPU0->CPU1 送信側想定）
 */
class CmdTx
{
public:
    explicit CmdTx(SharedIpc *s) : ring_(&s->cmd) {}
    bool Send(const Msg &m) { return ring_.Push(m); }

private:
    RingSpsc ring_;  // cmd リングの SPSC 操作
};

/**
 * @brief cmd リング受信（CPU1 側受信想定）
 */
class CmdRx
{
public:
    explicit CmdRx(SharedIpc *s) : ring_(&s->cmd) {}
    bool Recv(Msg &m) { return ring_.Pop(m); }

private:
    RingSpsc ring_;  // cmd リングの SPSC 操作
};

/**
 * @brief evt リング送信（CPU1->CPU0 送信側想定）
 */
class EvtTx
{
public:
    explicit EvtTx(SharedIpc *s) : ring_(&s->evt) {}
    bool Send(const Msg &m) { return ring_.Push(m); }

private:
    RingSpsc ring_;  // evt リングの SPSC 操作
};

/**
 * @brief evt リング受信（CPU0 側受信想定）
 */
class EvtRx
{
public:
    explicit EvtRx(SharedIpc *s) : ring_(&s->evt) {}
    bool Recv(Msg &m) { return ring_.Pop(m); }

private:
    RingSpsc ring_;  // evt リングの SPSC 操作
};
}  // namespace ipc
}  // namespace core
