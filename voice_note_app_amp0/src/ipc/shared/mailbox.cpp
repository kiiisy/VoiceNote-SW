/**
 * @file mailbox.cpp
 * @brief mailboxの実装
 */

// 自ヘッダー
#include "mailbox.h"

// Vitisライブラリ
#include "xpseudo_asm_gcc.h"

namespace core {
namespace ipc {

/**
 * @brief リングへ1メッセージpushする
 *
 * @param msg 送信するメッセージ
 * @return 空きがあればtrue、満杯ならfalse
 */
bool RingSpsc::Push(const Msg &msg)
{
    const uint32_t head = r_->head;
    const uint32_t tail = r_->tail;
    const uint32_t next = (head + 1u) & (kRingN - 1u);
    // Snext==tail は「1要素空け」方式で満杯を表す
    if (next == tail) {
        return false;
    }

    // 先にpayload本体を書いてからheadを進める
    r_->slots[head] = msg;

    // 書き込み順序を保証
    dmb();

    r_->head = next;

    // head公開の順序を確定
    dmb();

    return true;
}

/**
 * @brief リングから1メッセージpopする
 *
 * @param[out] msg 受信したメッセージ
 * @return データがあればtrue、空ならfalse
 */
bool RingSpsc::Pop(Msg &msg)
{
    const uint32_t head = r_->head;
    const uint32_t tail = r_->tail;
    // tail==head は空
    if (tail == head) {
        return false;
    }

    // head/tail スナップショットに基づき、現在の tail スロットを取り出す。
    msg = r_->slots[tail];

    const uint32_t next = (tail + 1u) & (kRingN - 1u);

    // 読み出し後にtailを進める順序を保証
    dmb();

    r_->tail = next;

    // tail公開の順序を確定
    dmb();

    return true;
}

}  // namespace ipc
}  // namespace core
