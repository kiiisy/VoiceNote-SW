/**
 * @file mailbox.cpp
 * @brief CPU0/CPU1 間の共有メモリ上 SPSC（Single Producer / Single Consumer）リング実装
 *
 * 共有メモリ（OCM High など）上に配置した Ring 構造体を用いて、
 * 片方向の SPSC キュー（producer 1、consumer 1）を lock-free に実現する。
 *
 * 前提:
 * - cmd: CPU0 -> CPU1（CPU0 が producer、CPU1 が consumer）
 * - evt: CPU1 -> CPU0（CPU1 が producer、CPU0 が consumer）
 *
 * メモリ順序:
 * - slots[] への書き込み/読み出しと head/tail 更新の間に dmb() を挿入し、
 *   共有メモリ上の可視性を保証する（ARMv7-A）。
 */

// 自ヘッダー
#include "mailbox.h"

// Vitisライブラリ
extern "C" {
#include "xpseudo_asm_gcc.h"
}

namespace core {
namespace ipc {

/**
 * @brief リングへ 1 メッセージ push する（producer 側）
 *
 * @param m 送信するメッセージ
 * @return 空きがあれば true、満杯なら false
 */
bool RingSpsc::Push(const Msg &m)
{
    const uint32_t head = r_->head;
    const uint32_t tail = r_->tail;
    const uint32_t next = (head + 1u) & (kRingN - 1u);

    if (next == tail) {
        return false;
    }

    r_->slots[head] = m;
    dmb();
    r_->head = next;
    dmb();

    return true;
}

/**
 * @brief リングから 1 メッセージ pop する（consumer 側）
 *
 * @param[out] m 受信したメッセージ
 * @return データがあれば true、空なら false
 */
bool RingSpsc::Pop(Msg &m)
{
    const uint32_t head = r_->head;
    const uint32_t tail = r_->tail;

    if (tail == head) {
        return false;
    }

    m = r_->slots[tail];

    const uint32_t next = (tail + 1u) & (kRingN - 1u);
    dmb();
    r_->tail = next;
    dmb();

    return true;
}
}  // namespace ipc
}  // namespace core
