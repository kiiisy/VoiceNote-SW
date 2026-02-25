/**
 * @file mailbox.h
 * @brief CPU0/CPU1共有メモリ上のIPC
 *
 * 共有メモリ上にSharedIpcを配置し、cmd/evtの2本のSPSCリングで
 * 双方向通信を行う。
 *
 * 構成:
 * - SharedIpc::cmd : CPU0 -> CPU1（CmdTx/CmdRx）
 * - SharedIpc::evt : CPU1 -> CPU0（EvtTx/EvtRx）
 */

#pragma once

// 共通ライブラリ
#include <cstdint>

// プロジェクトライブラリ
#include "ipc_msg.h"
#include "shared_mm.h"

namespace core {
namespace ipc {

static constexpr uint32_t kRingN = 256;  // リングスロット数
static_assert((kRingN & (kRingN - 1u)) == 0u, "kRingN must be power-of-two");

/**
 * @brief リング本体
 */
struct alignas(64) Ring
{
    volatile uint32_t head;           // producer
    volatile uint32_t tail;           // consumer
    uint32_t          _pad[14];       // パディング
    Msg               slots[kRingN];  // 固定長 Msg のスロット配列
};

/**
 * @brief 共有IPC領域のトップ構造体
 */
struct alignas(64) SharedIpc
{
    uint32_t magic;      // 共有領域の識別子
    uint32_t version;    // 構造体バージョン
    uint32_t _rsv0[14];  // 64B ヘッダ領域
    Ring     cmd;        // CPU0 -> CPU1
    Ring     evt;        // CPU1 -> CPU0
};
static_assert(sizeof(SharedIpc) <= kSharedSize, "SharedIpc exceeds reserved shared memory size");

/**
 * @brief 共有IPC領域へのポインタを取得する
 *
 * @return 共有メモリ上のSharedIpcポインタ
 */
inline SharedIpc *GetShared() { return reinterpret_cast<SharedIpc *>(ipc::kSharedBase); }

/**
 * @brief リング操作クラス
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
 * @brief cmdリング送信
 */
class CmdTx
{
public:
    explicit CmdTx(SharedIpc *s) : ring_(&s->cmd) {}
    bool Send(const Msg &m) { return ring_.Push(m); }

private:
    RingSpsc ring_;
};

/**
 * @brief cmdリング受信
 */
class CmdRx
{
public:
    explicit CmdRx(SharedIpc *s) : ring_(&s->cmd) {}
    bool Recv(Msg &m) { return ring_.Pop(m); }

private:
    RingSpsc ring_;
};

/**
 * @brief evtリング送信
 */
class EvtTx
{
public:
    explicit EvtTx(SharedIpc *s) : ring_(&s->evt) {}
    bool Send(const Msg &m) { return ring_.Push(m); }

private:
    RingSpsc ring_;
};

/**
 * @brief evtリング受信
 */
class EvtRx
{
public:
    explicit EvtRx(SharedIpc *s) : ring_(&s->evt) {}
    bool Recv(Msg &m) { return ring_.Pop(m); }

private:
    RingSpsc ring_;
};

}  // namespace ipc
}  // namespace core
